#pragma once
#include "config.h"
#include "fix_builds.h"
#include "hook_platform.h"
#include "hook_quiescence.h"
#include <array>

extern "C" void cmf_refresh_spread_bridge();
extern "C" unsigned char cmf_refresh_spread_bridge_end;
extern "C" void cmf_refresh_spread_bridge_v160();
extern "C" unsigned char cmf_refresh_spread_bridge_v160_end;
extern "C" volatile LONG cmf_refresh_spread_active;
// Published once before installation. Production's single owner is serialized by
// lifecycle_mutex and must fully stop before reconfiguration; retained on faults.
extern "C" volatile LONG cmf_refresh_spread_bucket_count;

namespace cmf {
class Logger;
namespace spread {
using Strategy=fix_builds::SpreadBridgeStrategy;
struct Layout {
    Strategy strategy;
    std::array<std::uint8_t,9> original;
    std::size_t span;
    std::uint8_t empty_displacement;
};
inline constexpr Layout legacy_layout{Strategy::legacy_v157_v159,{0x66,0x0f,0x1f,0x84,0,0,0,0,0},9,0x16};
inline constexpr Layout v160_layout{Strategy::v160_rcx_rbp,{0x0f,0x1f,0x84,0,0,0,0,0,0},8,0x15};
bool valid_layout(const Layout&) noexcept;
const Layout* layout_for(Strategy) noexcept;
inline constexpr std::uint32_t target_rva=0x003EC647,region_begin=0x003EC330,region_end=0x003EC677;
inline constexpr std::array<std::uint8_t,9> original{0x66,0x0f,0x1f,0x84,0,0,0,0,0};
inline constexpr std::array<std::uint8_t,75> signature{
 0x48,0x8b,0xac,0x24,0x80,0,0,0,0x74,0x2e,0x48,0x8b,0x5e,0x28,0x48,0x8b,0x46,0x30,
 0x48,0x8d,0x3c,0xc3,0x48,0x3b,0xdf,0x74,0x1d,0x66,0x0f,0x1f,0x84,0,0,0,0,0,
 0x48,0x8b,0x0b,0x48,0x8b,0xd6,0xe8,0x75,0,0x2e,0,0x48,0x83,0xc3,8,0x48,0x3b,0xdf,
 0x75,0xec,0x48,0x8b,0xbc,0x24,0x88,0,0,0,0x48,0x8b,0x5c,0x24,0x70,0x48,0x83,0xc4,0x60,0x5e,0xc3};
bool validate_image(const std::uint8_t*,std::size_t,
    const fix_builds::Descriptor* build=&fix_builds::ets157) noexcept;
// Internal bounded validator shared by exact production descriptors and authored
// tests. This is not a user-configurable address or signature fallback interface.
bool validate_site_image(const std::uint8_t*,std::size_t,std::uint32_t target,
    std::uint32_t begin,std::uint32_t end,const Layout&,fix_builds::SpreadSignature,
    std::uint32_t call_displacement) noexcept;

// One normal CALL/RET plus JRCXZ inside the exact ABI-owned padding. The lease
// protects its return into that padding until the bridge returns. No return
// address rewriting (including hardware shadow stacks), or relocated game load.
class Site final:public lifecycle::Operations {
    std::uint8_t* target_;
    MemoryRange game_;
    const Layout layout_;
    const std::uint32_t bucket_count_;
    std::array<std::uint8_t,9> detour_{};
    std::uint8_t* relay_=nullptr;
    DWORD protection_=0;
    bool pending_=false;
    WindowsPatchMemory memory_;
    SuspendedThreads suspended_;
public:
    const char* freeze_reason=nullptr;
    Site(std::uint8_t* target,MemoryRange game,Layout layout=legacy_layout,std::uint32_t bucket_count=60) noexcept:
        target_(target),game_(game),layout_(layout),bucket_count_(bucket_count),
        memory_(target,layout_.original.data(),detour_.data(),layout.span<=9?layout.span:0,protection_,pending_){}
    bool prepare() noexcept override;
    bool pin_module() noexcept override{return pin_hook_module();}
    void publish() noexcept override{InterlockedExchange(&cmf_refresh_spread_bucket_count,static_cast<LONG>(bucket_count_));}
    void recording(bool) noexcept override{}
    bool freeze() noexcept override;
    void thaw() noexcept override{suspended_.release();}
    bool no_active_calls() noexcept override{return InterlockedCompareExchange(&cmf_refresh_spread_active,0,0)==0;}
    lifecycle::PatchResult patch(bool install) noexcept override{return lifecycle::transact(memory_,install);}
    lifecycle::PatchResult repair_original_auxiliaries() noexcept override{return lifecycle::repair_original_auxiliaries(memory_);}
    void clear_continuation() noexcept override{}
    bool has_unwind() const noexcept override{return false;} // Static MASM .pdata, module PIN.
    bool has_allocation() const noexcept override{return relay_!=nullptr;}
    bool unregister_unwind() noexcept override{return true;}
    bool free_allocation() noexcept override;
    MemoryRange relay_range() const noexcept{return {reinterpret_cast<std::uintptr_t>(relay_),4096};}
    MemoryRange bridge_range() const noexcept;
};
}
class RefreshSpread final {
    lifecycle::Controller controller_;
    alignas(spread::Site) unsigned char storage_[sizeof(spread::Site)]{};
    spread::Site* site_=nullptr;
public:
    bool start(const PluginConfig&,bool exact_build,Logger&,
        const fix_builds::Descriptor* build=&fix_builds::ets157) noexcept;
    bool stop(Logger&) noexcept;
};
}
