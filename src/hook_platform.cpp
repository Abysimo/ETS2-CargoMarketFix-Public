#include "hook_platform.h"
#include <atomic>
#include <cstring>

namespace cmf {
namespace {
const char kPinAddressAnchor = 0;
std::atomic<bool> g_module_was_pinned{false};
}

bool pin_hook_module() noexcept {
    HMODULE module = nullptr;
    const bool pinned = GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&kPinAddressAnchor), &module) != FALSE;
    if (pinned) g_module_was_pinned.store(true, std::memory_order_release);
    return pinned;
}

bool hook_module_was_pinned() noexcept {
    return g_module_was_pinned.load(std::memory_order_acquire);
}

WindowsPatchMemory::WindowsPatchMemory(std::uint8_t* target,
    const HookBytes& original, const HookBytes& detour,
    DWORD& baseline_protection, bool& protection_pending) noexcept
    : WindowsPatchMemory(target,original.data(),detour.data(),original.size(),
        baseline_protection,protection_pending) {}
WindowsPatchMemory::WindowsPatchMemory(std::uint8_t* target,
    const std::uint8_t* original,const std::uint8_t* detour,std::size_t size,
    DWORD& baseline_protection,bool& protection_pending) noexcept
    : target_(target), original_(original), detour_(detour), size_(size),
      baseline_(baseline_protection), pending_(protection_pending) {}

lifecycle::Bytes WindowsPatchMemory::classify() noexcept {
    MEMORY_BASIC_INFORMATION region{};
    if (!target_ || VirtualQuery(target_, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
        reinterpret_cast<std::uintptr_t>(target_) -
            reinterpret_cast<std::uintptr_t>(region.BaseAddress) > region.RegionSize ||
        size_ > region.RegionSize -
            (reinterpret_cast<std::uintptr_t>(target_) -
             reinterpret_cast<std::uintptr_t>(region.BaseAddress))) {
        return lifecycle::Bytes::unknown;
    }
    if (std::memcmp(target_, original_, size_) == 0)
        return lifecycle::Bytes::original;
    if (std::memcmp(target_, detour_, size_) == 0)
        return lifecycle::Bytes::detour;
    return lifecycle::Bytes::unknown;
}
bool WindowsPatchMemory::writable() noexcept {
    DWORD old = 0;
    if (!VirtualProtect(target_, size_, PAGE_EXECUTE_READWRITE, &old))
        return false;
    if (!baseline_) baseline_ = old;
    pending_ = true;
    return true;
}
bool WindowsPatchMemory::write(lifecycle::Bytes bytes) noexcept {
    if (bytes == lifecycle::Bytes::unknown) return false;
    const auto& source = bytes == lifecycle::Bytes::original ? original_ : detour_;
    std::memcpy(target_, source, size_);
    return true;
}
bool WindowsPatchMemory::flush() noexcept {
    return FlushInstructionCache(GetCurrentProcess(), target_, size_) != FALSE;
}
bool WindowsPatchMemory::restore_protection() noexcept {
    if (!pending_) return true;
    DWORD ignored = 0;
    if (!baseline_ || !VirtualProtect(target_, size_, baseline_, &ignored))
        return false;
    pending_ = false;
    return true;
}
}  // namespace cmf
