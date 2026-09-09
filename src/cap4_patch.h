#pragma once
#include "config.h"
#include "fix_builds.h"
#include "hook_platform.h"
#include "hook_quiescence.h"
#include <array>

namespace cmf {
class Logger;
namespace cap4 {
inline constexpr std::uint32_t target_rva=0x006CE618;
inline constexpr std::uint32_t region_begin=0x006CE3F0,region_end=0x006CEE5B;
inline constexpr std::array<std::uint8_t,67> signature{
    0x48,0x83,0x7c,0x24,0x68,0x04,0xb9,0xff,0xff,0xff,0xff,0x48,0x89,0x75,0x40,
    0xc6,0x45,0x68,0x01,0x73,0x0a,0x48,0xc7,0x45,0x88,0x04,0,0,0,0xeb,0x24,
    0x48,0x83,0x7c,0x24,0x68,0x0a,0x76,0x0a,0x48,0xc7,0x45,0x88,0x0a,0,0,0,
    0xeb,0x12,0x48,0x8b,0x44,0x24,0x68,0x48,0x89,0x45,0x88,0x48,0x85,0xc0,
    0x0f,0x84,0x73,0x07,0,0};
// Loaded-image validation; never reads game objects or runs on the game hot path.
bool validate_image(const std::uint8_t* base,std::size_t size,
    const fix_builds::Descriptor* build=&fix_builds::ets157) noexcept;

// Existing Windows protection/cache backend, with the validated aligned atomic
// two-byte store. No callback/trampoline becomes callable from the game.
class Memory final:public lifecycle::PatchMemory {
    std::uint8_t* target_;
    DWORD protection_=0;
    bool pending_=false;
    inline static constexpr std::uint8_t original_[2]{0x73,0x0a},patched_[2]{0x90,0x90};
    WindowsPatchMemory platform_;
public:
    explicit Memory(std::uint8_t* target) noexcept:target_(target),
        platform_(target,original_,patched_,2,protection_,pending_){}
    lifecycle::Bytes classify() noexcept override {return platform_.classify();}
    bool writable() noexcept override {return platform_.writable();}
    bool write(lifecycle::Bytes bytes) noexcept override;
    bool flush() noexcept override {return platform_.flush();}
    bool restore_protection() noexcept override {return platform_.restore_protection();}
};

class Site final:public lifecycle::Operations {
    std::uint8_t* target_;
    Memory memory_;
    MemoryRange range_;
    SuspendedThreads suspended_;
public:
    const char* freeze_reason=nullptr;
    Site(std::uint8_t* target,MemoryRange range) noexcept:target_(target),memory_(target),range_(range){}
    bool prepare() noexcept override;
    bool pin_module() noexcept override {return pin_hook_module();}
    void publish() noexcept override {}
    void recording(bool) noexcept override {}
    bool freeze() noexcept override {return suspended_.acquire(std::array<MemoryRange,1>{range_},freeze_reason);}
    void thaw() noexcept override {suspended_.release();}
    // No new callable resources, and no calls inside the patched clamp window.
    // This is not an active-frame census. Suspended IPs exclude the whole RE003.
    bool no_active_calls() noexcept override {return true;}
    lifecycle::PatchResult patch(bool install) noexcept override {return lifecycle::transact(memory_,install);}
    void clear_continuation() noexcept override {}
    bool has_unwind() const noexcept override {return false;}
    bool has_allocation() const noexcept override {return false;}
    bool unregister_unwind() noexcept override {return true;}
    bool free_allocation() noexcept override {return true;}
};
}

class Cap4Patch final {
    lifecycle::Controller controller_;
    // Storage owned for the module lifetime, including failed restoration.
    alignas(cap4::Site) unsigned char storage_[sizeof(cap4::Site)]{};
    cap4::Site* site_=nullptr;
public:
    bool start(const PluginConfig&,bool exact_build,Logger&,
        const fix_builds::Descriptor* build=&fix_builds::ets157) noexcept;
    bool stop(Logger&) noexcept;
    bool potentially_live() const noexcept {return controller_.may_be_live();}
};
}
