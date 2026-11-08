#include "cap4_patch.h"
#include "build_info.h"
#include "logger.h"
#include <cstring>
#include <iomanip>
#include <new>
#include <sstream>

namespace cmf {
namespace {
bool readable(const void* address,std::size_t bytes) noexcept {
    auto p=reinterpret_cast<std::uintptr_t>(address);
    if(!p||bytes>UINTPTR_MAX-p)return false;
    const auto end=p+bytes;
    while(p<end){
        MEMORY_BASIC_INFORMATION m{};
        if(VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m))!=sizeof(m)||m.State!=MEM_COMMIT||
           (m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
        const auto prot=m.Protect&255;
        if(prot!=PAGE_READONLY&&prot!=PAGE_READWRITE&&prot!=PAGE_WRITECOPY&&
           prot!=PAGE_EXECUTE_READ&&prot!=PAGE_EXECUTE_READWRITE&&prot!=PAGE_EXECUTE_WRITECOPY)return false;
        const auto b=reinterpret_cast<std::uintptr_t>(m.BaseAddress);
        if(m.RegionSize>UINTPTR_MAX-b||b+m.RegionSize<=p)return false;
        p=(end<b+m.RegionSize)?end:b+m.RegionSize;
    }
    return true;
}
}
namespace cap4 {
bool validate_image(const std::uint8_t* base,std::size_t size,const fix_builds::Descriptor* build) noexcept {
    if(!fix_builds::known(build))return false;
    const auto* expected_signature=build->cap4_signature;
    const auto signature_size=build->cap4_signature_size;
    if(!base||size<sizeof(IMAGE_DOS_HEADER)||!readable(base,sizeof(IMAGE_DOS_HEADER)))return false;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0)return false;
    const auto off=static_cast<std::size_t>(dos->e_lfanew);
    if(off>size||sizeof(IMAGE_NT_HEADERS64)>size-off||!readable(base+off,sizeof(IMAGE_NT_HEADERS64)))return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+off);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
       nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt->OptionalHeader.SizeOfImage!=size||
       nt->FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)||
       !nt->FileHeader.NumberOfSections||nt->FileHeader.NumberOfSections>96||size<build->generator_end)return false;
    const auto so=off+sizeof(IMAGE_NT_HEADERS64);
    const auto sb=nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
    if(so>size||sb>size-so||!readable(base+so,sb))return false;
    const auto* sections=reinterpret_cast<const IMAGE_SECTION_HEADER*>(base+so);
    unsigned matches=0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
        const auto& s=sections[i];
        if(std::memcmp(s.Name,".text",5)||(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)==0)continue;
        const auto r=s.VirtualAddress,n=s.Misc.VirtualSize;
        if(r>size||n>size-r||n<signature_size||!readable(base+r,n))return false;
        for(std::size_t j=0;j<=n-signature_size;++j){
            if(base[r+j]!=expected_signature[0]||std::memcmp(base+r+j,expected_signature,signature_size))continue;
            if(++matches!=1||r+j!=build->cap4-19)return false;
        }
    }
    return matches==1;
}
bool Memory::writable() noexcept {
    if(strategy_==WriteStrategy::quiesced_copy2) {
        if(!quiesced_||!*quiesced_)return false; // Before VirtualProtect.
    } else if(strategy_!=WriteStrategy::aligned_atomic16)return false;
    return platform_.writable();
}
bool Memory::write(lifecycle::Bytes bytes) noexcept {
    if(strategy_==WriteStrategy::quiesced_copy2) {
        if(!quiesced_||!*quiesced_)return false;
        return platform_.write(bytes); // Exactly two bytes, alignment-independent.
    }
    if(strategy_!=WriteStrategy::aligned_atomic16)return false;
    if(!target_||(reinterpret_cast<std::uintptr_t>(target_)&1)||bytes==lifecycle::Bytes::unknown)return false;
    InterlockedExchange16(reinterpret_cast<volatile SHORT*>(target_),
        bytes==lifecycle::Bytes::original?static_cast<SHORT>(original_[0]|(original_[1]<<8)):SHORT(-28528));
    return true;
}
bool Site::prepare() noexcept {
    const auto p=reinterpret_cast<std::uintptr_t>(target_);
    const bool valid_strategy=strategy_==WriteStrategy::quiesced_copy2||
        (strategy_==WriteStrategy::aligned_atomic16&&!(p&1));
    return valid_strategy&&p!=UINTPTR_MAX&&range_.contains(p)&&range_.contains(p+1)&&
        memory_.classify()==lifecycle::Bytes::original;
}

void Site::log_prepatch_quiescence(Logger& log, MemoryRange image,
                                   const fix_builds::Descriptor& build) const noexcept {
    const auto& debug = quiescence_debug_;
    if (quiesced_ || !debug.snapshots_attempted || !debug.released ||
        (!debug.stable && !debug.failure_reason)) return;
    try {
        const auto pass_index = debug.snapshots_attempted - 1;
        const auto& last_pass = debug.passes[pass_index];
        std::ostringstream message;
        if (debug.stable) {
            message << "CAP4 quiescence debug: SUCCESS; passes=" << debug.snapshots_attempted
                    << "; process_threads=" << last_pass.process_threads_seen
                    << "; suspended=" << last_pass.suspended_total
                    << "; stable_pass=" << pass_index;
            log.write(message.str());
            return;
        }

        const auto hex = [&](std::uintptr_t value) {
            message << "0x" << std::uppercase << std::hex << std::setw(8)
                    << std::setfill('0') << value << std::dec;
        };
        message << "=== CAP4 QUIESCENCE DEBUG BEGIN ===\n"
                << "debug_version=" << kPluginVersion << "\nbuild=" << build.version
                << "\nexe_sha256=" << build.sha256 << "\ncap4_rva=";
        hex(build.cap4);
        message << "\ngenerator_begin=";
        hex(build.generator_begin);
        message << "\ngenerator_end=";
        hex(build.generator_end);
        message << "\nfailure_reason=" << debug.failure_reason << "\nwin32_error=";
        if (debug.has_win32_error) message << debug.win32_error;
        else message << "not_available";
        message << "\npass=" << pass_index << "\nprocess_id=" << debug.process_id
                << "\ncurrent_thread_id=" << debug.current_thread_id << "\ntarget_thread_id=";
        if (debug.target_thread_id) message << debug.target_thread_id;
        else message << "not_available";
        // A snapshot can outlive a thread. This error alone cannot prove that it did.
        const bool possible_exit = debug.has_win32_error &&
            debug.win32_error == ERROR_INVALID_PARAMETER &&
            std::strcmp(debug.failure_reason, "open_thread_failed") == 0;
        message << "\nthread_disappearance="
                << (possible_exit ? "possible_snapshot_exit" : "not_confirmed")
                << "\nsnapshots_attempted=" << debug.snapshots_attempted
                << "\nsuspended_total=" << last_pass.suspended_total
                << "\nmax_suspended_threads=" << kMaximumSuspendedThreads
                << "\nthread_set_stabilized=false\nrip=";
        if (debug.has_rip) {
            hex(debug.rip);
            message << "\nrva=";
            if (image.contains(debug.rip)) hex(debug.rip - image.begin);
            else message << "not_available";
            message << "\nforbidden_range_index=" << debug.forbidden_range_index
                    << "\nforbidden_range_begin=";
            hex(debug.forbidden_begin);
            message << "\nforbidden_range_end=";
            hex(debug.forbidden_begin + debug.forbidden_size);
            const MemoryRange patch{reinterpret_cast<std::uintptr_t>(target_), 2};
            message << "\nrip_region=" << (patch.contains(debug.rip) ? "cap4_patch_site" :
                range_.contains(debug.rip) ? "cap4_generator" : "other_forbidden_region");
        } else {
            message << "not_available\nrva=not_available\nforbidden_range_index=not_available"
                       "\nforbidden_range_begin=not_available\nforbidden_range_end=not_available"
                       "\nrip_region=not_available";
        }
        for (unsigned i = 0; i < debug.snapshots_attempted; ++i) {
            const auto& pass = debug.passes[i];
            message << "\npass=" << i << " entries_seen=" << pass.entries_seen
                    << " process_threads_seen=" << pass.process_threads_seen
                    << " known_threads_this_pass=" << pass.known_threads_seen
                    << " new_threads_this_pass=" << pass.new_threads_seen
                    << " opened=" << pass.opened << " suspended=" << pass.suspended
                    << " contexts_read=" << pass.contexts_read
                    << " suspended_total=" << pass.suspended_total;
        }
        message << "\n=== CAP4 QUIESCENCE DEBUG END ===";
        log.write(message.str());
    } catch (...) {
        // Diagnostic output is best effort and cannot change the patch result.
    }
}
}
bool Cap4Patch::start(const PluginConfig& config,bool exact_build,Logger& log,const fix_builds::Descriptor* build) noexcept {
    if(!config.cap4_install&&!config.cap4_enabled)return true;
    log.write("CAP4 requested; behavior-changing research patch; budget=4");
    if(const auto* refusal=config.cap4_refusal()){
        log.write(std::string("CAP4 refused: ")+refusal);return false;
    }
    if(!exact_build||!fix_builds::known(build)){log.write("CAP4 refused: exact build mismatch");return false;}
    log.write("CAP4 exact build matched");
    if(site_||controller_.phase()!=lifecycle::Phase::empty){log.write("CAP4 refused: existing owner state");return false;}
    auto* base=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if(!readable(base,sizeof(IMAGE_DOS_HEADER))){log.write("CAP4 refused: image header");return false;}
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x100000||
       !readable(base+dos->e_lfanew,sizeof(IMAGE_NT_HEADERS64))){log.write("CAP4 refused: PE header");return false;}
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    const auto image_size = nt->OptionalHeader.SizeOfImage;
    if(!cap4::validate_image(base,image_size,build)){log.write("CAP4 refused: target signature/bytes mismatch");return false;}
    log.write(std::string("CAP4 target bytes matched; build=")+build->version+
        "; RVA_decimal="+std::to_string(build->cap4)+"; original_word_decimal="+
        std::to_string(build->cap4_original)+" -> 90 90");
    site_=new(storage_) cap4::Site(base+build->cap4,
        {reinterpret_cast<std::uintptr_t>(base)+build->generator_begin,build->generator_end-build->generator_begin},
        build->cap4_original,build->cap4_write_strategy);
    const bool installed = controller_.install(true,*site_);
    site_->log_prepatch_quiescence(log,
        {reinterpret_cast<std::uintptr_t>(base), image_size}, *build);
    if(!installed){
        log.write(std::string("CAP4 installation refused: ")+controller_.reason());return false;
    }
    log.write("CAP4 installed; budget=4");return true;
}
bool Cap4Patch::stop(Logger& log) noexcept {
    if(!site_)return true;
    const bool was_live=controller_.may_be_live();
    const bool ok=controller_.stop(*site_);
    if(!ok){log.write(std::string("CAP4 restoration unconfirmed; owner retained: ")+controller_.reason());return false;}
    if(was_live)log.write("CAP4 restored; exact original build bytes");
    if(controller_.release_allowed()){
        site_->~Site();site_=nullptr;controller_=lifecycle::Controller{};
    }
    return true;
}
}
