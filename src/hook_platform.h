#pragma once
#include "hook_lifecycle.h"
#include <windows.h>
#include <array>
#include <cstdint>

namespace cmf {
using HookBytes = std::array<std::uint8_t, 15>;

// Pin the module containing this implementation, NOT the process executable.
// A PIN is intentionally irreversible until process termination.
bool pin_hook_module() noexcept;
bool hook_module_was_pinned() noexcept;

class WindowsPatchMemory final : public lifecycle::PatchMemory {
public:
    WindowsPatchMemory(std::uint8_t* target, const HookBytes& original,
                       const HookBytes& detour, DWORD& baseline_protection,
                       bool& protection_pending) noexcept;
    WindowsPatchMemory(std::uint8_t* target, const std::uint8_t* original,
                       const std::uint8_t* detour, std::size_t size,
                       DWORD& baseline_protection, bool& protection_pending) noexcept;
    lifecycle::Bytes classify() noexcept override;
    bool writable() noexcept override;
    bool write(lifecycle::Bytes bytes) noexcept override;
    bool flush() noexcept override;
    bool restore_protection() noexcept override;
private:
    std::uint8_t* target_;
    const std::uint8_t* original_;
    const std::uint8_t* detour_;
    std::size_t size_;
    DWORD& baseline_;
    bool& pending_;
};
}  // namespace cmf
