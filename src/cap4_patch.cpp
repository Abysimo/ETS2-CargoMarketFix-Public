#include "cap4_patch.h"
#include "logger.h"
#include <cstring>
#include <new>

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
        if(r>size||n>size-r||n<signature.size()||!readable(base+r,n))return false;
        for(std::size_t j=0;j<=n-signature.size();++j){
            if(base[r+j]!=signature[0]||std::memcmp(base+r+j,signature.data(),signature.size()))continue;
            if(++matches!=1||r+j!=build->cap4-19)return false;
        }
    }
    return matches==1;
}
bool Memory::write(lifecycle::Bytes bytes) noexcept {
    if(!target_||(reinterpret_cast<std::uintptr_t>(target_)&1)||bytes==lifecycle::Bytes::unknown)return false;
    InterlockedExchange16(reinterpret_cast<volatile SHORT*>(target_),
        bytes==lifecycle::Bytes::original?SHORT(0x0a73):SHORT(-28528));
    return true;
}
bool Site::prepare() noexcept {
    const auto p=reinterpret_cast<std::uintptr_t>(target_);
    return !(p&1)&&range_.contains(p)&&range_.contains(p+1)&&memory_.classify()==lifecycle::Bytes::original;
}
}
bool Cap4Patch::start(const PluginConfig& config,bool exact_build,Logger& log,const fix_builds::Descriptor* build) noexcept {
    if(!config.cap4_install&&!config.cap4_enabled)return true;
    log.write("CAP4 requested; generation budget=4");
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
    if(!cap4::validate_image(base,nt->OptionalHeader.SizeOfImage,build)){log.write("CAP4 refused: target signature/bytes mismatch");return false;}
    log.write(std::string("CAP4 target bytes matched; build=")+build->version+
        "; RVA_decimal="+std::to_string(build->cap4)+"; 73 0A -> 90 90");
    site_=new(storage_) cap4::Site(base+build->cap4,
        {reinterpret_cast<std::uintptr_t>(base)+build->generator_begin,build->generator_end-build->generator_begin});
    if(!controller_.install(true,*site_)){
        log.write(std::string("CAP4 installation refused: ")+controller_.reason());return false;
    }
    log.write("CAP4 installed; budget=4");return true;
}
bool Cap4Patch::stop(Logger& log) noexcept {
    if(!site_)return true;
    const bool was_live=controller_.may_be_live();
    const bool ok=controller_.stop(*site_);
    if(!ok){log.write(std::string("CAP4 restoration unconfirmed; owner retained: ")+controller_.reason());return false;}
    if(was_live)log.write("CAP4 restored; original bytes=73 0A");
    if(controller_.release_allowed()){
        site_->~Site();site_=nullptr;controller_=lifecycle::Controller{};
    }
    return true;
}
}
