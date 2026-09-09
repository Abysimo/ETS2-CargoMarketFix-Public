#include "refresh_spread.h"
#include "logger.h"
#include <cstring>
#include <new>

extern "C" {volatile LONG cmf_refresh_spread_active=0;}
namespace cmf {
namespace spread {
static bool readable(const void* pointer,std::size_t size) noexcept {
    auto p=reinterpret_cast<std::uintptr_t>(pointer);
    if(!p||size>UINTPTR_MAX-p)return false;
    const auto end=p+size;
    while(p<end){MEMORY_BASIC_INFORMATION m{};
        if(!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m))||m.State!=MEM_COMMIT||
            (m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
        const auto protection=m.Protect&255;
        if(protection!=PAGE_READONLY&&protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY&&
            protection!=PAGE_EXECUTE_READ&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)return false;
        const auto b=reinterpret_cast<std::uintptr_t>(m.BaseAddress);
        if(m.RegionSize>UINTPTR_MAX-b||b+m.RegionSize<=p)return false;
        p=(end<b+m.RegionSize)?end:b+m.RegionSize;
    }return true;
}
bool validate_image(const std::uint8_t* base,std::size_t size,const fix_builds::Descriptor* build) noexcept {
    if(!fix_builds::known(build))return false;
    auto expected=spread::signature;
    std::memcpy(expected.data()+43,&build->sweep_call_displacement,4);
    if(size<build->sweep_end||!readable(base,sizeof(IMAGE_DOS_HEADER)))return false;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0)return false;
    const auto off=static_cast<std::size_t>(dos->e_lfanew);
    if(off>size||sizeof(IMAGE_NT_HEADERS64)>size-off||!readable(base+off,sizeof(IMAGE_NT_HEADERS64)))return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+off);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
        nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt->OptionalHeader.SizeOfImage!=size||
        nt->FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)||
        !nt->FileHeader.NumberOfSections||nt->FileHeader.NumberOfSections>96)return false;
    const auto so=off+sizeof(IMAGE_NT_HEADERS64),sn=nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
    if(so>size||sn>size-so||!readable(base+so,sn))return false;
    const auto* sections=reinterpret_cast<const IMAGE_SECTION_HEADER*>(base+so);
    unsigned matches=0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto& s=sections[i];
        if(std::memcmp(s.Name,".text",5)||!(s.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        const auto r=s.VirtualAddress,n=s.Misc.VirtualSize;
        if(r>size||n>size-r||n<expected.size()||!readable(base+r,n))return false;
        for(std::size_t j=0;j<=n-expected.size();++j)
            if(base[r+j]==expected[0]&&!std::memcmp(base+r+j,expected.data(),expected.size()))
                if(++matches!=1||r+j!=build->spread-27)return false;
    }return matches==1;
}
bool Site::prepare() noexcept {
    if(relay_||memory_.classify()!=lifecycle::Bytes::original)return false;
    const auto p=reinterpret_cast<std::uintptr_t>(target_);
    if(!game_.contains(p)||!game_.contains(p+8))return false;
    // Allocate at allocation-granularity steps, within signed CALL rel32 reach.
    const auto anchor=p&~std::uintptr_t(65535);
    for(std::uintptr_t d=65536;d<0x70000000&&!relay_;d+=65536){
        const std::uintptr_t candidates[2]{anchor>=d?anchor-d:0,anchor+d};
        for(auto a:candidates){if(!a||a>UINTPTR_MAX-4096)continue;
            relay_=static_cast<std::uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(a),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
            if(relay_)break;
        }
    }
    if(!relay_)return false;
    const auto delta=reinterpret_cast<std::intptr_t>(relay_)-static_cast<std::intptr_t>(p+5);
    if(delta<INT32_MIN||delta>INT32_MAX)return false;
    relay_[0]=0xff;relay_[1]=0x25;std::memset(relay_+2,0,4);
    const auto bridge=reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge);
    std::memcpy(relay_+6,&bridge,8);
    DWORD old=0;
    if(!VirtualProtect(relay_,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay_,14))return false;
    detour_.fill(0x90);detour_[0]=0xe8;detour_[5]=0xe3;detour_[6]=0x16;
    const auto displacement=static_cast<std::int32_t>(delta);std::memcpy(detour_.data()+1,&displacement,4);
    return true;
}
bool Site::freeze() noexcept {
    const auto bridge=reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge);
    return suspended_.acquire(std::array<MemoryRange,3>{game_,relay_range(),
        MemoryRange{bridge,reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge_end)-bridge}},freeze_reason);
}
bool Site::free_allocation() noexcept {
    if(!relay_)return true;
    if(!VirtualFree(relay_,0,MEM_RELEASE))return false;
    relay_=nullptr;return true;
}
}
bool RefreshSpread::start(const PluginConfig& config,bool exact_build,Logger& log,const fix_builds::Descriptor* build) noexcept {
    if(!config.refresh_spread_install&&!config.refresh_spread_enabled)return true;
    if(const auto* reason=config.refresh_spread_refusal()){
        log.write(std::string("Refresh spread refused: ")+reason);return false;}
    if(!exact_build||!fix_builds::known(build)||site_){log.write("Refresh spread refused: build/owner gate");return false;}
    auto* base=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if(!spread::readable(base,sizeof(IMAGE_DOS_HEADER)))return false;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x100000||
        !spread::readable(base+dos->e_lfanew,sizeof(IMAGE_NT_HEADERS64)))return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(!spread::validate_image(base,nt->OptionalHeader.SizeOfImage,build)){
        log.write("Refresh spread refused: exact sweep signature");return false;}
    site_=new(storage_) spread::Site(base+build->spread,
        {reinterpret_cast<std::uintptr_t>(base)+build->sweep_begin,build->sweep_end-build->sweep_begin});
    if(!controller_.install(true,*site_)){
        log.write(std::string("Refresh spread install failed: ")+controller_.reason());return false;}
    log.write(std::string("CMF refresh spread ACTIVE; contract=1; minutes=60; partition=contiguous; normal-only; build=")+build->version+
        "; gate_RVA_decimal="+std::to_string(build->spread)+"; CAP4 retained; no backlog; bulk/activation unchanged");
    return true;
}
bool RefreshSpread::stop(Logger& log) noexcept {
    if(!site_)return true;
    const bool ok=controller_.stop(*site_);
    if(!ok){log.write(std::string("Refresh spread restoration unconfirmed; containment: ")+controller_.reason());return false;}
    if(controller_.release_allowed()){
        site_->~Site();site_=nullptr;controller_=lifecycle::Controller{};
        log.write("CMF refresh spread restored; original sweep padding verified; callable relay released; no retained traversal state");
    }return true;
}
}
