#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <array>
#include <cstdint>
#include <cstddef>
namespace cmf {
constexpr std::size_t kMaximumSuspendedThreads=256;
struct MemoryRange final {
    std::uintptr_t begin = 0;
    std::size_t size = 0;

    [[nodiscard]] bool contains(const std::uintptr_t address) const noexcept {
        return address >= begin && address - begin < size;
    }
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
                 const char*& reason) noexcept {
        release();
        const DWORD process_id = GetCurrentProcessId();
        const DWORD current_thread_id = GetCurrentThreadId();
        // Require a subsequent complete snapshot with no unseen process
        // threads. Bound both storage and closure attempts; never treat an
        // enumeration error as end-of-list. No process attachment is used.
        for (unsigned pass = 0; pass < 4; ++pass) {
            const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot == INVALID_HANDLE_VALUE) {
                reason = "thread_snapshot_failed"; release(); return false;
            }
            THREADENTRY32 entry{};
            entry.dwSize = sizeof(entry);
            if (!Thread32First(snapshot, &entry)) {
                CloseHandle(snapshot); reason = "thread_enumeration_failed";
                release(); return false;
            }
            bool success = true, added = false;
            do {
                if (entry.th32OwnerProcessID != process_id ||
                    entry.th32ThreadID == current_thread_id) continue;
                bool known = false;
                for (std::size_t i = 0; i < count_; ++i)
                    if (ids_[i] == entry.th32ThreadID) known = true;
                if (known) continue;
                if (count_ >= handles_.size()) {
                    reason = "too_many_threads_for_bounded_freeze"; success = false; break;
                }
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                    THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
                if (!thread) { reason = "open_thread_failed"; success = false; break; }
                if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
                    CloseHandle(thread); reason = "suspend_thread_failed"; success = false; break;
                }
                ids_[count_] = entry.th32ThreadID;
                handles_[count_++] = thread;
                added = true;
                CONTEXT context{};
                context.ContextFlags = CONTEXT_CONTROL;
                if (!GetThreadContext(thread, &context)) {
                    reason = "get_thread_context_failed"; success = false; break;
                }
#if defined(_M_X64)
                const auto instruction_pointer = static_cast<std::uintptr_t>(context.Rip);
#else
#error CargoMarketFix observation hooks require an x64 build.
#endif
                for (const MemoryRange& range : forbidden) {
                    if (range.contains(instruction_pointer)) {
                        reason = "thread_inside_patch_or_trampoline_range"; success = false; break;
                    }
                }
                if (!success) break;
            } while (Thread32Next(snapshot, &entry));
            if (success && GetLastError() != ERROR_NO_MORE_FILES) {
                reason = "thread_enumeration_incomplete"; success = false;
            }
            CloseHandle(snapshot);
            if (!success) { release(); return false; }
            if (pass != 0 && !added) return true;
        }
        reason = "thread_set_did_not_stabilize";
        release(); return false;
    }

    void release() noexcept {
        while (count_ != 0) {
            HANDLE thread = handles_[--count_];
            ResumeThread(thread);
            CloseHandle(thread);
            handles_[count_] = nullptr;
            ids_[count_] = 0;
        }
    }

private:
    std::array<HANDLE, kMaximumSuspendedThreads> handles_{};
    std::array<DWORD, kMaximumSuspendedThreads> ids_{};
    std::size_t count_ = 0;
};


} // namespace cmf
