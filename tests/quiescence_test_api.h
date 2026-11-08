#pragma once
#include <windows.h>
#include <tlhelp32.h>

// Force-included only in the authored fault-injection executable.
namespace quiescence_test {
DWORD process_id() noexcept;
DWORD thread_id() noexcept;
HANDLE snapshot(DWORD flags, DWORD pid) noexcept;
BOOL first(HANDLE, THREADENTRY32*) noexcept;
BOOL next(HANDLE, THREADENTRY32*) noexcept;
HANDLE open_thread(DWORD access, BOOL inherit, DWORD tid) noexcept;
DWORD suspend(HANDLE) noexcept;
BOOL context(HANDLE, CONTEXT*) noexcept;
DWORD resume(HANDLE) noexcept;
BOOL close(HANDLE) noexcept;
}

#define GetCurrentProcessId quiescence_test::process_id
#define GetCurrentThreadId quiescence_test::thread_id
#define CreateToolhelp32Snapshot quiescence_test::snapshot
#define Thread32First quiescence_test::first
#define Thread32Next quiescence_test::next
#define OpenThread quiescence_test::open_thread
#define SuspendThread quiescence_test::suspend
#define GetThreadContext quiescence_test::context
#define ResumeThread quiescence_test::resume
#define CloseHandle quiescence_test::close
