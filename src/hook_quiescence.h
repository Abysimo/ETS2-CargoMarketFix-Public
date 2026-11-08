#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <array>
#include <cstdint>
#include <cstddef>
namespace cmf {
constexpr std::size_t kMaximumSuspendedThreads=256;
constexpr unsigned kMaximumQuiescencePasses = 4;
struct MemoryRange final {
    std::uintptr_t begin = 0;
    std::size_t size = 0;

    [[nodiscard]] bool contains(const std::uintptr_t address) const noexcept {
        return address >= begin && address - begin < size;
    }
};

// CAP4 startup diagnostics only. Nothing here owns resources or allocates.
struct QuiescencePass final {
    std::uint64_t entries_seen;
    std::uint64_t process_threads_seen; // Includes the current thread.
    std::uint64_t known_threads_seen;
    std::uint64_t new_threads_seen; // Includes a thread refused before OpenThread.
    unsigned opened;
    unsigned suspended;
    unsigned contexts_read;
    std::size_t suspended_total;
};

struct QuiescenceDebug final {
    std::array<QuiescencePass, kMaximumQuiescencePasses> passes;
    unsigned snapshots_attempted;
    DWORD process_id;
    DWORD current_thread_id;
    DWORD target_thread_id;
    const char* failure_reason; // Points to the existing static reason literal.
    DWORD win32_error;
    bool has_win32_error;
    std::uintptr_t rip;
    std::size_t forbidden_range_index;
    std::uintptr_t forbidden_begin;
    std::size_t forbidden_size;
    bool has_rip;
    bool stable;
    unsigned resume_failures;
    bool released;
};

class SuspendedThreads final {
public:
    SuspendedThreads() = default;
    ~SuspendedThreads() {
        release();
    }

    SuspendedThreads(const SuspendedThreads&) = delete;
    SuspendedThreads& operator=(const SuspendedThreads&) = delete;

    template <std::size_t Count>
    bool acquire(const std::array<MemoryRange, Count>& forbidden,
                 const char*& reason, QuiescenceDebug* debug = nullptr) noexcept {
        release();
        const DWORD process_id = GetCurrentProcessId();
        const DWORD current_thread_id = GetCurrentThreadId();
        if (debug) {
            *debug = {};
            debug->process_id = process_id;
            debug->current_thread_id = current_thread_id;
        }
        const auto refuse = [&](const char* why, DWORD tid = 0, DWORD error = 0,
                                bool has_error = false) noexcept {
            reason = why;
            if (debug) {
                debug->failure_reason = why;
                debug->target_thread_id = tid;
                debug->win32_error = error;
                debug->has_win32_error = has_error;
            }
        };
        // Require a subsequent complete snapshot with no unseen process
        // threads. Bound both storage and closure attempts; never treat an
        // enumeration error as end-of-list. No process attachment is used.
        for (unsigned pass = 0; pass < kMaximumQuiescencePasses; ++pass) {
            auto* counts = debug ? &debug->passes[pass] : nullptr;
            if (counts) {
                ++debug->snapshots_attempted;
                counts->suspended_total = count_;
            }
            const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot == INVALID_HANDLE_VALUE) {
                refuse("thread_snapshot_failed", 0, debug ? GetLastError() : 0, true);
                release(debug);
                return false;
            }
            THREADENTRY32 entry{};
            entry.dwSize = sizeof(entry);
            if (!Thread32First(snapshot, &entry)) {
                refuse("thread_enumeration_failed", 0, debug ? GetLastError() : 0, true);
                CloseHandle(snapshot);
                release(debug);
                return false;
            }
            bool success = true, added = false;
            do {
                if (counts) ++counts->entries_seen;
                if (entry.th32OwnerProcessID != process_id) continue;
                if (counts) ++counts->process_threads_seen;
                if (entry.th32ThreadID == current_thread_id) continue;
                bool known = false;
                for (std::size_t i = 0; i < count_; ++i)
                    if (ids_[i] == entry.th32ThreadID) known = true;
                if (known) {
                    if (counts) ++counts->known_threads_seen;
                    continue;
                }
                if (counts) ++counts->new_threads_seen;
                if (count_ >= handles_.size()) {
                    refuse("too_many_threads_for_bounded_freeze", entry.th32ThreadID);
                    success = false;
                    break;
                }
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                    THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
                if (!thread) {
                    refuse("open_thread_failed", entry.th32ThreadID, debug ? GetLastError() : 0, true);
                    success = false;
                    break;
                }
                if (counts) ++counts->opened;
                if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
                    refuse("suspend_thread_failed", entry.th32ThreadID, debug ? GetLastError() : 0, true);
                    CloseHandle(thread);
                    success = false;
                    break;
                }
                ids_[count_] = entry.th32ThreadID;
                handles_[count_++] = thread;
                added = true;
                if (counts) {
                    ++counts->suspended;
                    counts->suspended_total = count_;
                }
                CONTEXT context{};
                context.ContextFlags = CONTEXT_CONTROL;
                if (!GetThreadContext(thread, &context)) {
                    refuse("get_thread_context_failed", entry.th32ThreadID, debug ? GetLastError() : 0, true);
                    success = false;
                    break;
                }
                if (counts) ++counts->contexts_read;
#if defined(_M_X64)
                const auto instruction_pointer = static_cast<std::uintptr_t>(context.Rip);
#else
#error CargoMarketFix observation hooks require an x64 build.
#endif
                for (std::size_t index = 0; index < forbidden.size(); ++index) {
                    const auto& range = forbidden[index];
                    if (range.contains(instruction_pointer)) {
                        refuse("thread_inside_patch_or_trampoline_range", entry.th32ThreadID);
                        if (debug) {
                            debug->has_rip = true;
                            debug->rip = instruction_pointer;
                            debug->forbidden_range_index = index;
                            debug->forbidden_begin = range.begin;
                            debug->forbidden_size = range.size;
                        }
                        success = false;
                        break;
                    }
                }
                if (!success) break;
            } while (Thread32Next(snapshot, &entry));
            if (success) {
                const auto error = GetLastError();
                if (error != ERROR_NO_MORE_FILES) {
                    refuse("thread_enumeration_incomplete", 0, error, true);
                    success = false;
                }
            }
            CloseHandle(snapshot);
            if (!success) { release(debug); return false; }
            if (pass != 0 && !added) {
                if (debug) debug->stable = true;
                return true;
            }
        }
        refuse("thread_set_did_not_stabilize");
        release(debug);
        return false;
    }

    void release(QuiescenceDebug* debug = nullptr) noexcept {
        while (count_ != 0) {
            HANDLE thread = handles_[--count_];
            const auto previous_suspend_count = ResumeThread(thread);
            if (debug && previous_suspend_count == static_cast<DWORD>(-1))
                ++debug->resume_failures;
            CloseHandle(thread);
            handles_[count_] = nullptr;
            ids_[count_] = 0;
        }
        if (debug) debug->released = debug->resume_failures == 0;
    }

private:
    std::array<HANDLE, kMaximumSuspendedThreads> handles_{};
    std::array<DWORD, kMaximumSuspendedThreads> ids_{};
    std::size_t count_ = 0;
};


} // namespace cmf
