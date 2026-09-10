#pragma once
#include "config.h"
#include "fix_builds.h"
#include "hook_platform.h"
#include "hook_quiescence.h"
#include <array>

namespace cmf {
class Logger;
namespace cap4 {
using WriteStrategy=fix_builds::Cap4WriteStrategy;
inline constexpr std::uint32_t target_rva=0x006CE618;
inline constexpr std::uint32_t region_begin=0x006CE3F0,region_end=0x006CEE5B;
// Legacy fixture alias. Production always uses its exact identified descriptor.
inline constexpr auto& signature=fix_builds::cap4_legacy_signature;
// Loaded-image validation; never reads game objects or runs on the game hot path.
bool validate_image(const std::uint8_t* base,std::size_t size,
    const fix_builds::Descriptor* build=&fix_builds::ets157) noexcept;

// Exact metadata selects atomic16 or a two-byte copy under owner quiescence.
// No callback/trampoline becomes callable from the game.
class Memory final:public lifecycle::PatchMemory {
    std::uint8_t* target_;
    DWORD protection_=0;
    bool pending_=false;
    const std::uint8_t original_[2]; // Owner-local backing survives containment.
    inline static constexpr std::uint8_t patched_[2]{0x90,0x90};
    WindowsPatchMemory platform_;
    const WriteStrategy strategy_;
    const bool* const quiesced_;
public:
    explicit Memory(std::uint8_t* target,std::uint16_t original=0x0a73,
        WriteStrategy strategy=WriteStrategy::aligned_atomic16,const bool* quiesced=nullptr) noexcept:target_(target),
        original_{static_cast<std::uint8_t>(original),static_cast<std::uint8_t>(original>>8)},
        platform_(target,original_,patched_,2,protection_,pending_),strategy_(strategy),quiesced_(quiesced){}
    lifecycle::Bytes classify() noexcept override {return platform_.classify();}
    bool writable() noexcept override;
    bool write(lifecycle::Bytes bytes) noexcept override;
    bool flush() noexcept override {return platform_.flush();}
    bool restore_protection() noexcept override {return platform_.restore_protection();}
};

class Site final:public lifecycle::Operations {
    std::uint8_t* target_;
    const WriteStrategy strategy_;
    bool quiesced_=false; // Owner-local; Memory can only read this authorization.
    Memory memory_;
    MemoryRange range_;
    SuspendedThreads suspended_;
public:
    const char* freeze_reason=nullptr;
    Site(std::uint8_t* target,MemoryRange range,std::uint16_t original=0x0a73,
        WriteStrategy strategy=WriteStrategy::aligned_atomic16) noexcept:
        target_(target),strategy_(strategy),memory_(target,original,strategy,&quiesced_),range_(range){}
    ~Site() override {thaw();}
    bool quiesced() const noexcept {return quiesced_;}
    bool prepare() noexcept override;
    bool pin_module() noexcept override {return pin_hook_module();}
    void publish() noexcept override {}
    void recording(bool) noexcept override {}
    bool freeze() noexcept override {
        quiesced_=false;
        quiesced_=suspended_.acquire(std::array<MemoryRange,1>{range_},freeze_reason);
        return quiesced_;
    }
    void thaw() noexcept override {quiesced_=false;suspended_.release();}
    // No new callable resources, and no calls inside the patched clamp window.
    // This is not an active-frame census. Suspended IPs exclude the whole RE003.
    bool no_active_calls() noexcept override {return true;}
    lifecycle::PatchResult patch(bool install) noexcept override {return lifecycle::transact(memory_,install);}
    lifecycle::PatchResult repair_original_auxiliaries() noexcept override {return lifecycle::repair_original_auxiliaries(memory_);}
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
