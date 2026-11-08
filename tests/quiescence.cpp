#include "cap4_patch.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

using namespace cmf;
namespace {
enum class Fault { none, snapshot, first, next, open, suspend, context, rip };

constexpr DWORD pid = 10, self = 20, first_worker = 100;
constexpr DWORD failed_call = static_cast<DWORD>(-1);
constexpr std::uintptr_t snapshot_handle = 1;
constexpr auto worker_slots = kMaximumSuspendedThreads + 1;

struct TransientSpan { DWORD first; unsigned count; };
struct Scenario {
    Fault fault;
    std::array<unsigned, kMaximumQuiescencePasses> workers;
    unsigned fault_pass = 0;
    DWORD tid = first_worker + 1;
    DWORD error = ERROR_ACCESS_DENIED;
    std::uintptr_t rip = 0;
    std::array<TransientSpan, kMaximumQuiescencePasses> transient{};
};

struct ThreadState { bool open, suspended; };
struct SimulatedThreads {
    Scenario scenario{};
    std::array<ThreadState, worker_slots> handles{};
    unsigned snapshots = 0, pass = 0, entry = 0;
    unsigned open_count = 0, suspended_count = 0, resume_count = 0;
    bool snapshot_open = false, invalid_call = false;
    std::array<unsigned, kMaximumQuiescencePasses> suspended_at_snapshot{};
    std::array<unsigned, kMaximumQuiescencePasses> contexts_by_pass{};
    const std::uint8_t* patch_during_resume = nullptr;
    std::uint16_t expected_patch_word = 0;

    bool fail(Fault stage, DWORD tid = 0) const noexcept {
        return scenario.fault == stage && scenario.fault_pass == pass &&
               (!tid || scenario.tid == tid);
    }

    ThreadState* handle_state(HANDLE h) noexcept {
        const auto tid = reinterpret_cast<std::uintptr_t>(h);
        if (tid >= first_worker && tid - first_worker < handles.size()) {
            auto& state = handles[tid - first_worker];
            if (state.open) return &state;
        }
        invalid_call = true;
        return nullptr;
    }

    void fill(THREADENTRY32& out) const noexcept {
        out.th32OwnerProcessID = entry == 0 ? pid + 1 : pid;
        out.th32ThreadID = entry == 0 ? self + 1 :
            entry == 1 ? self : first_worker + entry - 2;
    }
} sim;
}

namespace quiescence_test {
DWORD process_id() noexcept { return pid; }
DWORD thread_id() noexcept { return self; }
HANDLE snapshot(DWORD flags, DWORD process) noexcept {
    if (flags != TH32CS_SNAPTHREAD || process || sim.snapshot_open ||
        sim.snapshots >= kMaximumQuiescencePasses) {
        sim.invalid_call = true;
        return INVALID_HANDLE_VALUE;
    }
    sim.pass = sim.snapshots++;
    sim.suspended_at_snapshot[sim.pass] = sim.suspended_count;
    if (sim.fail(Fault::snapshot)) {
        SetLastError(sim.scenario.error);
        return INVALID_HANDLE_VALUE;
    }
    sim.snapshot_open = true;
    return reinterpret_cast<HANDLE>(snapshot_handle);
}
BOOL first(HANDLE h, THREADENTRY32* entry) noexcept {
    if (!sim.snapshot_open || reinterpret_cast<std::uintptr_t>(h) != snapshot_handle ||
        entry->dwSize != sizeof(*entry)) sim.invalid_call = true;
    if (sim.fail(Fault::first)) {
        SetLastError(sim.scenario.error);
        return FALSE;
    }
    sim.entry = 0;
    sim.fill(*entry);
    return TRUE;
}
BOOL next(HANDLE h, THREADENTRY32* entry) noexcept {
    if (!sim.snapshot_open || reinterpret_cast<std::uintptr_t>(h) != snapshot_handle)
        sim.invalid_call = true;
    if (++sim.entry == sim.scenario.workers[sim.pass] + 2) {
        SetLastError(sim.fail(Fault::next) ? sim.scenario.error : ERROR_NO_MORE_FILES);
        return FALSE;
    }
    sim.fill(*entry);
    return TRUE;
}
HANDLE open_thread(DWORD access, BOOL inherit, DWORD tid) noexcept {
    if (access != (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION) ||
        inherit || tid < first_worker || tid - first_worker >= sim.handles.size() ||
        sim.handles[tid - first_worker].open) {
        sim.invalid_call = true;
        return nullptr;
    }
    if (sim.fail(Fault::open, tid)) {
        SetLastError(sim.scenario.error);
        return nullptr;
    }
    const auto& transient = sim.scenario.transient[sim.pass];
    if (tid >= transient.first && tid - transient.first < transient.count) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    sim.handles[tid - first_worker].open = true;
    ++sim.open_count;
    return reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(tid));
}
DWORD suspend(HANDLE h) noexcept {
    auto* state = sim.handle_state(h);
    if (!state) return failed_call;
    if (state->suspended) sim.invalid_call = true;
    if (sim.fail(Fault::suspend, static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(h)))) {
        SetLastError(sim.scenario.error);
        return failed_call;
    }
    state->suspended = true;
    ++sim.suspended_count;
    return 0;
}
BOOL context(HANDLE h, CONTEXT* registers) noexcept {
    const auto* state = sim.handle_state(h);
    if (!state || !state->suspended || registers->ContextFlags != CONTEXT_CONTROL) {
        sim.invalid_call = true;
        return FALSE;
    }
    const auto tid = static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(h));
    if (sim.fail(Fault::context, tid)) {
        SetLastError(sim.scenario.error);
        return FALSE;
    }
    registers->Rip = sim.fail(Fault::rip, tid) ? sim.scenario.rip : 1;
    ++sim.contexts_by_pass[sim.pass];
    return TRUE;
}
DWORD resume(HANDLE h) noexcept {
    auto* state = sim.handle_state(h);
    if (!state || !state->suspended) {
        sim.invalid_call = true;
        return failed_call;
    }
    if (const auto* patch = sim.patch_during_resume) {
        MEMORY_BASIC_INFORMATION page{};
        if (static_cast<std::uint16_t>(patch[0] | (patch[1] << 8)) != sim.expected_patch_word ||
            VirtualQuery(patch, &page, sizeof(page)) != sizeof(page) ||
            page.Protect != PAGE_EXECUTE_READ) sim.invalid_call = true;
    }
    ++sim.resume_count;
    state->suspended = false;
    --sim.suspended_count;
    return 1;
}
BOOL close(HANDLE h) noexcept {
    if (reinterpret_cast<std::uintptr_t>(h) == snapshot_handle) {
        if (!sim.snapshot_open) sim.invalid_call = true;
        sim.snapshot_open = false;
    } else if (auto* state = sim.handle_state(h)) {
        if (state->suspended) sim.invalid_call = true;
        state->open = false;
        --sim.open_count;
    }
    // Catch code that collects the error after closing the snapshot or thread.
    SetLastError(ERROR_INVALID_HANDLE);
    return TRUE;
}
}

namespace {
unsigned cases = 0;
void need(bool ok, const char* label) {
    if (!ok) throw std::runtime_error(label);
}
void pass(const char* label) {
    ++cases;
    std::printf("PASS %s\n", label);
}
struct Fixture {
    std::uint8_t* base = static_cast<std::uint8_t*>(
        VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    Fixture() {
        need(base, "fixture allocation");
        const unsigned char code[]{0xb8,4,0,0,0,0x83,0xf9,4,0x90,0x73,0x0b,
            0xb8,4,0,0,0,0xc3,0x90,0x90,0x90,0x90,0x90,0xb8,10,0,0,0,0xc3};
        std::memcpy(base, code, sizeof(code));
        DWORD old = 0;
        need(VirtualProtect(base,4096,PAGE_EXECUTE_READ,&old) != 0, "fixture RX");
        need(FlushInstructionCache(GetCurrentProcess(),base,4096) != 0, "fixture flush");
    }
    ~Fixture() { VirtualFree(base, 0, MEM_RELEASE); }
    MemoryRange range() const { return {reinterpret_cast<std::uintptr_t>(base), 64}; }
    MemoryRange image() const { return {range().begin, 4096}; }
    std::uint8_t* patch_site() const { return base + 9; }
    bool original() const {
        return base[8] == 0x90 && base[9] == 0x73 && base[10] == 0x0b && base[11] == 0xb8;
    }
    bool patched() const {
        return base[8] == 0x90 && base[9] == 0x90 && base[10] == 0x90 && base[11] == 0xb8;
    }
    unsigned budget() const { return reinterpret_cast<unsigned(*)(unsigned)>(base)(7); }
};

Scenario with_transients(Scenario scenario, std::initializer_list<TransientSpan> passes) {
    need(passes.size() <= scenario.transient.size(), "fixture pass limit");
    std::copy(passes.begin(), passes.end(), scenario.transient.begin());
    return scenario;
}

void verify_cleanup(unsigned resumed) {
    need(!sim.invalid_call && sim.resume_count == resumed && !sim.snapshot_open &&
         !sim.open_count && !sim.suspended_count &&
         std::none_of(sim.handles.begin(), sim.handles.end(),
             [](const ThreadState& h) { return h.open || h.suspended; }),
         "all owned threads resumed exactly once and handles closed");
}

void verify_recovery(Fixture& f, Scenario scenario,
                     std::initializer_list<unsigned> contexts_by_pass, const char* label) {
    sim = {};
    sim.scenario = scenario;
    sim.patch_during_resume = f.patch_site();
    sim.expected_patch_word = 0x9090;
    cap4::Site site(f.patch_site(), f.range(), 0x0b73, cap4::WriteStrategy::quiesced_copy2);
    lifecycle::Controller controller;
    need(controller.install(true,site) && sim.snapshots == contexts_by_pass.size(),
         "CAP4 waits for the clean pass");
    unsigned held = 0, i = 0;
    for (const auto checked : contexts_by_pass) {
        need(sim.suspended_at_snapshot[i] == held && sim.contexts_by_pass[i] == checked,
             "retained suspensions and inspected each new live thread");
        held += checked;
        ++i;
    }
    need(f.patched() && f.budget() == 4 && !site.quiesced() &&
         controller.last_patch().cache_synchronized && controller.last_patch().protection_restored,
         "patch and verification complete before thaw");
    verify_cleanup(held);

    sim = {};
    sim.scenario.workers = {1,1,1,1};
    sim.patch_during_resume = f.patch_site();
    sim.expected_patch_word = 0x0b73;
    need(controller.stop(site) && controller.original_verified() &&
         f.original() && f.budget() == 10 && !site.quiesced(), "exact restore before thaw");
    verify_cleanup(1);
    pass(label);
}

void verify_refusal(Fixture& f, Scenario scenario, unsigned passes, unsigned resumed,
                    const char* reason, const char* label) {
    sim = {};
    sim.scenario = scenario;
    sim.patch_during_resume = f.patch_site();
    sim.expected_patch_word = 0x0b73;
    cap4::Site site(f.patch_site(), f.range(), 0x0b73, cap4::WriteStrategy::quiesced_copy2);
    lifecycle::Controller controller;
    need(!controller.install(true,site) && sim.snapshots == passes &&
         std::strcmp(controller.reason(),"prepatch_quiescence_failed_never_exposed") == 0 &&
         site.freeze_reason && std::strcmp(site.freeze_reason,reason) == 0,
         "expected fail-closed reason and snapshot count");
    need(f.original() && f.budget() == 10 && !site.quiesced() && !controller.may_be_live() &&
         !controller.last_patch().write_attempted && controller.release_allowed(),
         "target remains untouched and no patch is authorized");
    verify_cleanup(resumed);
    pass(label);
}
}

int main() {
    try {
        static_assert(kMaximumQuiescencePasses == 4);
        Fixture f;
        verify_recovery(f,with_transients({Fault::none,{105,104,0,0}},{{204,1}}),
            {104,0},"external_sequence_then_clean_snapshot");
        verify_recovery(f,with_transients({Fault::none,{2,2,2,0}},{{101,1}}),
            {1,1,0},"reused_failed_tid_is_opened_suspended_and_checked");
        verify_recovery(f,with_transients({Fault::none,{2,2,2,1}},{{101,1},{101,1},{101,1}}),
            {1,0,0,0},"repeated_transients_then_clean_fourth_pass");
        verify_recovery(f,{Fault::none,{1,2,2,0}},
            {1,1,0},"ordinary_stabilization_unchanged");
        verify_refusal(f,with_transients({Fault::none,{2,2,2,2}},{{101,1},{101,1},{101,1},{101,1}}),
            4,1,"thread_set_did_not_stabilize","transient_churn_hits_pass_limit");
        verify_refusal(f,{Fault::open,{2,2,2,2},0,101,ERROR_ACCESS_DENIED},
            1,1,"open_thread_failed","access_denied_is_fatal");
        verify_refusal(f,{Fault::open,{2,2,2,2},0,101,ERROR_INVALID_HANDLE},
            1,1,"open_thread_failed","other_open_error_is_fatal");
        verify_refusal(f,{Fault::suspend,{2,2,2,2},0,101,ERROR_INVALID_PARAMETER},
            1,1,"suspend_thread_failed","suspend_error_87_is_fatal");
        verify_refusal(f,with_transients(
                {Fault::context,{3,3,3,3},0,102,ERROR_INVALID_PARAMETER},{{101,1}}),
            1,2,"get_thread_context_failed","context_error_87_after_transient_is_fatal");
        verify_refusal(f,with_transients(
                {Fault::rip,{3,3,3,3},0,102,0,f.range().begin+20},{{101,1}}),
            1,2,"thread_inside_patch_or_trampoline_range","forbidden_rip_after_transient_is_fatal");
        verify_refusal(f,with_transients({Fault::next,{2,1,1,1},1,0,ERROR_INVALID_DATA},{{101,1}}),
            2,1,"thread_enumeration_incomplete","incomplete_followup_pass_is_fatal");
        verify_refusal(f,with_transients({Fault::snapshot,{2,1,1,1},1,0,ERROR_INVALID_PARAMETER},{{101,1}}),
            2,1,"thread_snapshot_failed","snapshot_error_87_is_fatal");
        verify_refusal(f,{Fault::first,{1,1,1,1},1,0,ERROR_NO_MORE_FILES},
            2,1,"thread_enumeration_failed","first_enumeration_failure_is_fatal");
        verify_refusal(f,{Fault::none,{257,0,0,0}},
            1,256,"too_many_threads_for_bounded_freeze","capacity_is_fatal");
        verify_refusal(f,{Fault::none,{1,2,3,4}},
            4,4,"thread_set_did_not_stabilize","unseen_live_thread_churn_hits_limit");
        std::printf("QUIESCENCE_TOTAL %u\n", cases);
        return 0;
    } catch (const std::exception& error) {
        std::printf("FAIL %s\n", error.what());
        return 1;
    }
}
