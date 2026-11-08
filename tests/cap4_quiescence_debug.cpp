#include "cap4_patch.h"
#include "build_info.h"
#include "logger.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>

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
    bool fail_resume = false;
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
    if (sim.scenario.fail_resume) return failed_call;
    state->suspended = false;
    --sim.suspended_count;
    return 1;
}
BOOL close(HANDLE h) noexcept {
    if (reinterpret_cast<std::uintptr_t>(h) == snapshot_handle) {
        if (!sim.snapshot_open) sim.invalid_call = true;
        sim.snapshot_open = false;
    } else if (auto* state = sim.handle_state(h)) {
        if (state->suspended && !sim.scenario.fail_resume) sim.invalid_call = true;
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

std::string read_log(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

void verify_failure(Fixture& fixture, Logger& log, Scenario scenario,
                    const char* reason, unsigned snapshots, DWORD target_tid,
                    bool has_error, unsigned total_suspended, const QuiescencePass& expected,
                    const char* log_detail = nullptr, unsigned total_transient = 0) {
    sim = {};
    sim.scenario = scenario;
    const auto before = read_log(log.path()).size();
    cap4::Site site(fixture.patch_site(),fixture.range(),0x0b73,cap4::WriteStrategy::quiesced_copy2);
    lifecycle::Controller controller;
    need(!controller.install(true,site), "prepatch failure refused");
    need(std::strcmp(controller.reason(),"prepatch_quiescence_failed_never_exposed") == 0 &&
         !controller.may_be_live() && !controller.last_patch().write_attempted &&
         fixture.original() && fixture.budget() == 10 && !site.quiesced() &&
         controller.release_allowed(), "no patch or live owner after refusal");
    const auto& debug = site.quiescence_debug();
    need(debug.failure_reason && std::strcmp(debug.failure_reason,reason) == 0 &&
         site.freeze_reason == debug.failure_reason && debug.snapshots_attempted == snapshots &&
         debug.target_thread_id == target_tid && debug.process_id == pid &&
         debug.current_thread_id == self && !debug.stable &&
         debug.has_win32_error == has_error &&
         (!has_error || debug.win32_error == scenario.error), "failure record attribution");
    need(debug.has_rip == (scenario.fault == Fault::rip), "RIP availability");
    if (debug.has_rip) {
        need(debug.rip == scenario.rip && debug.forbidden_range_index == 0 &&
             debug.forbidden_begin == fixture.range().begin &&
             debug.forbidden_size == fixture.range().size, "RIP and range attribution");
    }
    const auto& observed = debug.passes[snapshots - 1];
    need(observed.suspended_total == total_suspended &&
         observed.entries_seen == expected.entries_seen &&
         observed.process_threads_seen == expected.process_threads_seen &&
         observed.known_threads_seen == expected.known_threads_seen &&
         observed.new_threads_seen == expected.new_threads_seen &&
         observed.opened == expected.opened && observed.suspended == expected.suspended &&
         observed.contexts_read == expected.contexts_read &&
         observed.transient_open_thread_87 == expected.transient_open_thread_87 &&
         debug.transient_open_thread_87_total == total_transient, "last pass and transient counts");
    need(!sim.invalid_call && sim.snapshots == snapshots && sim.open_count == 0 &&
         !sim.snapshot_open && sim.resume_count == total_suspended, "exact handle cleanup");
    need(debug.released == !scenario.fail_resume &&
         sim.suspended_count == (scenario.fail_resume ? total_suspended : 0),
         "all owned suspensions released unless resume fails");
    need(read_log(log.path()).size() == before, "no freeze-time logging");
    site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619,false);
    const auto output = read_log(log.path()).substr(before);
    if (scenario.fail_resume) {
        need(output.empty() && debug.resume_failures == total_suspended,
             "suppress diagnostic output if cleanup unconfirmed");
        pass("failed_resume_suppresses_debug_block");
        return;
    }
    need(output.find("=== CAP4 QUIESCENCE DEBUG BEGIN ===") != std::string::npos &&
         output.find("=== CAP4 QUIESCENCE DEBUG END ===") != std::string::npos &&
         output.find(std::string("failure_reason=") + reason) != std::string::npos &&
         output.find(std::string("win32_error=") + (has_error ?
             std::to_string(scenario.error) : "not_available")) != std::string::npos &&
         output.find(std::string("target_thread_id=") + (target_tid ?
             std::to_string(target_tid) : "not_available")) != std::string::npos &&
         output.find(std::string("debug_version=") + kPluginVersion) != std::string::npos &&
         output.find(fix_builds::ets1619.sha256) != std::string::npos &&
         output.find("cap4_rva=0x007CFE05") != std::string::npos &&
         output.find("generator_begin=0x007CFBA0") != std::string::npos &&
         output.find("generator_end=0x007D08EC") != std::string::npos &&
         output.find("stabilization_pass=not_available") != std::string::npos &&
         output.find("success_required_transient_recovery=false") != std::string::npos &&
         output.find("patch_installed=false") != std::string::npos &&
         output.find("transient_open_thread_87_total=" + std::to_string(total_transient) + "\r\n") !=
             std::string::npos &&
         (!log_detail || output.find(log_detail) != std::string::npos), "post-release debug block");
    pass(reason);
}

Scenario with_transients(Scenario scenario, std::initializer_list<TransientSpan> passes) {
    need(passes.size() <= scenario.transient.size(), "fixture pass limit");
    std::copy(passes.begin(), passes.end(), scenario.transient.begin());
    return scenario;
}

void verify_recovery(Fixture& fixture, Logger& log, Scenario scenario,
                     std::initializer_list<unsigned> opened_by_pass, const char* label) {
    sim = {};
    sim.scenario = scenario;
    sim.patch_during_resume = fixture.patch_site();
    sim.expected_patch_word = 0x9090;
    const auto before = read_log(log.path()).size();
    cap4::Site site(fixture.patch_site(),fixture.range(),0x0b73,cap4::WriteStrategy::quiesced_copy2);
    lifecycle::Controller controller;
    need(controller.install(true,site), "CAP4 installs after clean stabilization");
    const auto& debug = site.quiescence_debug();
    need(debug.stable && debug.released && !debug.failure_reason && !site.quiesced() &&
         debug.snapshots_attempted == opened_by_pass.size(), "clean stabilization and thaw");

    unsigned held = 0, transient_total = 0, i = 0;
    std::string transient_ids, transient_by_pass;
    for (const unsigned opened : opened_by_pass) {
        const auto& pass = debug.passes[i];
        const auto& transient = scenario.transient[i];
        need(sim.suspended_at_snapshot[i] == held &&
             pass.known_threads_seen == held &&
             pass.entries_seen == scenario.workers[i] + 2 &&
             pass.process_threads_seen == scenario.workers[i] + 1 &&
             pass.new_threads_seen == opened + transient.count &&
             pass.transient_open_thread_87 == transient.count &&
             pass.opened == opened && pass.suspended == opened && pass.contexts_read == opened,
             "suspensions retained and all new live TIDs inspected");
        held += opened;
        need(pass.suspended_total == held, "total held after pass");
        if (i != 0) transient_by_pass += ',';
        transient_by_pass += std::to_string(i++) + ':' + std::to_string(transient.count);
        for (unsigned j = 0; j < transient.count; ++j) {
            if (transient_total < debug.transient_thread_ids.size()) {
                const DWORD tid = transient.first + j;
                need(debug.transient_thread_ids[transient_total] == tid, "recorded transient TID");
                if (transient_total != 0) transient_ids += ',';
                transient_ids += std::to_string(tid);
            }
            ++transient_total;
        }
    }
    need(debug.transient_open_thread_87_total == transient_total &&
         debug.passes[i-1].transient_open_thread_87 == 0 &&
         debug.passes[i-1].new_threads_seen == 0, "final pass has no transient or new thread");
    need(fixture.patched() && fixture.budget() == 4 && controller.last_patch().cache_synchronized &&
         controller.last_patch().protection_restored &&
         !sim.invalid_call && sim.resume_count == held && !sim.open_count &&
         !sim.suspended_count && !sim.snapshot_open, "write/cache/protection then thaw without leaks");
    need(read_log(log.path()).size() == before, "no logging during install");
    site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619,true);
    const auto output = read_log(log.path()).substr(before);
    const auto omitted = transient_total > kMaximumLoggedTransientThreadIds
        ? transient_total - kMaximumLoggedTransientThreadIds : 0;
    need(output.find("CAP4 quiescence debug: SUCCESS_AFTER_TRANSIENT_THREAD_EXIT;") != std::string::npos &&
         output.find("total_snapshot_passes=" + std::to_string(i) + "\r\n") != std::string::npos &&
         output.find("stabilization_pass=" + std::to_string(i - 1) + "\r\n") != std::string::npos &&
         output.find("success_required_transient_recovery=true") != std::string::npos &&
         output.find("patch_installed=true") != std::string::npos &&
         output.find("final_suspended_total=" + std::to_string(held) + "\r\n") != std::string::npos &&
         output.find("transient_open_thread_87_total=" + std::to_string(transient_total) + "\r\n") != std::string::npos &&
         output.find("transient_open_thread_87_by_pass=" + transient_by_pass + "\r\n") != std::string::npos &&
         output.find("transient_thread_ids=" + transient_ids + "\r\n") != std::string::npos &&
         output.find("transient_thread_ids_omitted=" + std::to_string(omitted) + "\r\n") != std::string::npos,
         "post-thaw recovery log");

    sim = {};
    sim.scenario.workers = {1,1,1,1};
    sim.patch_during_resume = fixture.patch_site();
    sim.expected_patch_word = 0x0b73;
    need(controller.stop(site) && controller.original_verified() &&
         fixture.original() && fixture.budget() == 10 && !sim.invalid_call &&
         sim.resume_count == 1 && !sim.open_count && !sim.suspended_count && !sim.snapshot_open,
         "fresh quiescence then exact restore and thaw");
    pass(label);
}
}

int main(int argc, char** argv) {
    try {
        static_assert(std::is_trivial_v<QuiescenceDebug> && std::is_standard_layout_v<QuiescenceDebug>);
        if (argc != 2) return 2;
        Logger log;
        need(log.open(argv[1],{}), "open offline fixture log");
        log.configure(0, 1024 * 1024);
        Fixture fixture;
        verify_failure(fixture,log,{Fault::snapshot,{1,1,1,1},1,0,ERROR_NOT_ENOUGH_MEMORY},
            "thread_snapshot_failed",2,0,true,1,{});
        verify_failure(fixture,log,{Fault::first,{1,1,1,1},1,0,ERROR_NO_MORE_FILES},
            "thread_enumeration_failed",2,0,true,1,{});
        verify_failure(fixture,log,{Fault::next,{2,2,2,2},0,0,ERROR_INVALID_DATA},
            "thread_enumeration_incomplete",1,0,true,2,{4,3,0,2,2,2,2,2});
        verify_failure(fixture,log,{Fault::none,{257,0,0,0}},
            "too_many_threads_for_bounded_freeze",1,356,false,256,{259,258,0,257,256,256,256,256});
        verify_failure(fixture,log,{Fault::open,{1,2,2,2},1},
            "open_thread_failed",2,101,true,1,{4,3,1,1,0,0,0,1},
            "thread_disappearance=not_confirmed");
        verify_failure(fixture,log,{Fault::open,{2,2,2,2},0,101,ERROR_INVALID_HANDLE},
            "open_thread_failed",1,101,true,1,{4,3,0,2,1,1,1,1},
            "thread_disappearance=not_confirmed");
        verify_failure(fixture,log,{Fault::suspend,{2,2,2,2},0,101,ERROR_INVALID_PARAMETER},
            "suspend_thread_failed",1,101,true,1,{4,3,0,2,2,1,1,1});
        verify_failure(fixture,log,{Fault::context,{2,2,2,2},0,101,ERROR_GEN_FAILURE},
            "get_thread_context_failed",1,101,true,2,{4,3,0,2,2,2,1,2});
        verify_failure(fixture,log,{Fault::rip,{2,2,2,2},0,101,0,fixture.range().begin + 9},
            "thread_inside_patch_or_trampoline_range",1,101,false,2,{4,3,0,2,2,2,2,2},
            "rva=0x00000009");
        verify_failure(fixture,log,{Fault::rip,{2,2,2,2},0,101,0,fixture.range().begin + 20},
            "thread_inside_patch_or_trampoline_range",1,101,false,2,{4,3,0,2,2,2,2,2},
            "rip_region=cap4_generator");
        verify_failure(fixture,log,{Fault::none,{1,2,3,4}},
            "thread_set_did_not_stabilize",4,0,false,4,{6,5,3,1,1,1,1,4});
        verify_failure(fixture,log,{Fault::context,{2,2,2,2},0,101,ERROR_GEN_FAILURE,0,true},
            "get_thread_context_failed",1,101,true,2,{4,3,0,2,2,2,1,2});

        verify_recovery(fixture,log,with_transients({Fault::none,{97,96,0,0}},{{196,1}}),
            {96,0},"reported_96_suspended_then_clean_pass");
        verify_recovery(fixture,log,with_transients({Fault::none,{2,2,2,0}},{{101,1}}),
            {1,1,0},"reused_failed_tid_open_suspend_context_check");
        verify_recovery(fixture,log,with_transients({Fault::none,{2,2,2,1}},{{101,1},{101,1},{101,1}}),
            {1,0,0,0},"repeated_87_passes_cannot_stabilize");
        verify_recovery(fixture,log,with_transients({Fault::none,{32,1,0,0}},{{101,31}}),
            {1,0},"bounded_tid_log_retains_total");
        verify_failure(fixture,log,with_transients({Fault::none,{2,2,2,2}},
                {{101,1},{101,1},{101,1},{101,1}}),
            "thread_set_did_not_stabilize",4,0,false,1,{4,3,1,1,0,0,0,1,1},
            "transient_open_thread_87_by_pass=0:1,1:1,2:1,3:1",4);
        verify_failure(fixture,log,with_transients(
                {Fault::rip,{3,3,3,3},0,102,0,fixture.range().begin+20},{{101,1}}),
            "thread_inside_patch_or_trampoline_range",1,102,false,2,{5,4,0,3,2,2,2,2,1},
            "rip_region=cap4_generator",1);
        verify_failure(fixture,log,with_transients(
                {Fault::context,{3,3,3,3},0,102,ERROR_INVALID_PARAMETER},{{101,1}}),
            "get_thread_context_failed",1,102,true,2,{5,4,0,3,2,2,1,2,1},nullptr,1);
        verify_failure(fixture,log,with_transients(
                {Fault::next,{2,1,1,1},1,0,ERROR_INVALID_DATA},{{101,1}}),
            "thread_enumeration_incomplete",2,0,true,1,{3,2,1,0,0,0,0,1,0},
            "transient_open_thread_87_by_pass=0:1,1:0",1);

        {
            sim = {};
            sim.scenario = with_transients({Fault::none,{2,1,1,1}},{{101,1}});
            SuspendedThreads threads;
            const char* reason = nullptr;
            QuiescenceDebug debug{};
            need(!threads.acquire(std::array<MemoryRange,1>{fixture.range()},reason,&debug) &&
                 std::strcmp(reason,"open_thread_failed") == 0 && debug.win32_error == ERROR_INVALID_PARAMETER &&
                 debug.target_thread_id == 101 && !debug.transient_open_thread_87_total &&
                 sim.snapshots == 1 && sim.resume_count == 1 && !sim.open_count &&
                 !sim.suspended_count && !sim.snapshot_open && !sim.invalid_call,
                 "default policy keeps strict OpenThread refusal");
            pass("strict_default_preserves_spread_quiescence");
        }

        sim = {};
        sim.scenario.workers = {1,2,2,2};
        cap4::Site site(fixture.patch_site(),fixture.range(),0x0b73,cap4::WriteStrategy::quiesced_copy2);
        const auto before = read_log(log.path()).size();
        need(site.freeze(), "stable native path");
        const auto& debug = site.quiescence_debug();
        need(debug.stable && !debug.released && !debug.failure_reason &&
             debug.snapshots_attempted == 3 && debug.passes[0].new_threads_seen == 1 &&
             debug.passes[1].known_threads_seen == 1 && debug.passes[1].new_threads_seen == 1 &&
             debug.passes[2].process_threads_seen == 3 && debug.passes[2].new_threads_seen == 0 &&
             debug.passes[2].known_threads_seen == 2 && debug.passes[2].suspended_total == 2 &&
             sim.suspended_count == 2, "complete stabilization counters");
        site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619,false);
        need(read_log(log.path()).size() == before, "log guarded while frozen");
        need(site.patch(true).committed && fixture.budget() == 4, "original CAP4 write semantics");
        site.thaw();
        need(!sim.invalid_call && !sim.open_count && !sim.suspended_count && !sim.snapshot_open &&
             sim.resume_count == 2 && debug.released, "success release precedes logging");
        site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619,true);
        need(read_log(log.path()).substr(before).find(
            "CAP4 quiescence debug: SUCCESS; passes=3; process_threads=3; suspended=2; stable_pass=2") !=
            std::string::npos, "post-release success summary");
        sim = {};
        sim.scenario.workers = {1,1,1,1};
        need(site.freeze() && site.patch(false).committed, "fresh restore freeze");
        site.thaw();
        need(fixture.original() && fixture.budget() == 10 &&
             !sim.invalid_call && !sim.open_count && !sim.suspended_count, "exact restore");
        pass("stable_patch_restore_and_post_thaw_success_log");
        std::printf("CAP4_QUIESCENCE_DEBUG_TOTAL %u\n", cases);
        return 0;
    } catch (const std::exception& error) {
        std::printf("FAIL %s\n", error.what());
        return 1;
    }
}
