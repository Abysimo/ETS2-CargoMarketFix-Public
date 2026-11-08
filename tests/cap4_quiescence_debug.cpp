#include "cap4_patch.h"
#include "build_info.h"
#include "logger.h"
#include <cstdio>
#include <cstring>
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

struct Scenario {
    Fault fault;
    std::array<unsigned, kMaximumQuiescencePasses> workers;
    unsigned fault_pass = 0;
    DWORD tid = first_worker + 1;
    DWORD error = ERROR_ACCESS_DENIED;
    std::uintptr_t rip = 0;
    bool fail_resume = false;
};

struct ThreadState { bool open, suspended; };
struct SimulatedThreads {
    Scenario scenario{};
    std::array<ThreadState, worker_slots> handles{};
    unsigned snapshots = 0, pass = 0, entry = 0;
    unsigned open_count = 0, suspended_count = 0, resume_count = 0;
    bool snapshot_open = false, invalid_call = false;

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
    unsigned budget() const { return reinterpret_cast<unsigned(*)(unsigned)>(base)(7); }
};

std::string read_log(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

void verify_failure(Fixture& fixture, Logger& log, Scenario scenario,
                    const char* reason, unsigned snapshots, DWORD target_tid,
                    bool has_error, unsigned total_suspended, const QuiescencePass& expected,
                    const char* log_detail = nullptr) {
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
         observed.contexts_read == expected.contexts_read, "last pass counts");
    need(!sim.invalid_call && sim.snapshots == snapshots && sim.open_count == 0 &&
         !sim.snapshot_open && sim.resume_count == total_suspended, "exact handle cleanup");
    need(debug.released == !scenario.fail_resume &&
         sim.suspended_count == (scenario.fail_resume ? total_suspended : 0),
         "all owned suspensions released unless resume fails");
    need(read_log(log.path()).size() == before, "no freeze-time logging");
    site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619);
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
         (!log_detail || output.find(log_detail) != std::string::npos), "post-release debug block");
    pass(reason);
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
        verify_failure(fixture,log,{Fault::open,{2,2,2,2},0,101,ERROR_INVALID_PARAMETER},
            "open_thread_failed",1,101,true,1,{4,3,0,2,1,1,1,1},
            "thread_disappearance=possible_snapshot_exit");
        verify_failure(fixture,log,{Fault::suspend,{2,2,2,2}},
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
        site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619);
        need(read_log(log.path()).size() == before, "log guarded while frozen");
        need(site.patch(true).committed && fixture.budget() == 4, "original CAP4 write semantics");
        site.thaw();
        need(!sim.invalid_call && !sim.open_count && !sim.suspended_count && !sim.snapshot_open &&
             sim.resume_count == 2 && debug.released, "success release precedes logging");
        site.log_prepatch_quiescence(log,fixture.image(),fix_builds::ets1619);
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
