#include "cap4_patch.h"
#include "refresh_spread.h"
#include "logger.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

static unsigned cases;
static void check(bool ok,const std::string& name){
    if(!ok)throw std::runtime_error(name);
    ++cases;std::printf("PASS EP%02u_%s\n",cases,name.c_str());
}
int main(){try{
    using namespace cmf;
    check(fix_builds::identify(fix_builds::ets157.sha256)==&fix_builds::ets157,"157_exact_identity");
    check(fix_builds::identify(fix_builds::ets158.sha256)==&fix_builds::ets158,"158_exact_identity");
    check(!fix_builds::identify("")&&!fix_builds::identify("1.58.1.4")&&!fix_builds::identify("UNKNOWN"),"unknown_hash_fail_closed");
    auto corrupted=std::string(fix_builds::ets158.sha256);corrupted.back()='0';
    check(!fix_builds::identify(corrupted),"one_hash_digit_fail_closed");
    auto forged=fix_builds::ets158;
    check(!fix_builds::known(&forged),"arbitrary_descriptor_rejected");
    constexpr std::size_t size=0x710000;
    auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!image)throw std::runtime_error("allocation");
    for(const auto* build:{&fix_builds::ets157,&fix_builds::ets158}){
        const auto tag=std::string(build->version)+"_";
        std::memset(image,0,size);
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+0x100);nt->Signature=IMAGE_NT_SIGNATURE;
        nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt->FileHeader.NumberOfSections=1;
        nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
        nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=size;
        auto* s=IMAGE_FIRST_SECTION(nt);std::memcpy(s->Name,".text",5);
        s->VirtualAddress=0x1000;s->Misc.VirtualSize=size-0x1000;s->Characteristics=IMAGE_SCN_MEM_EXECUTE;
        std::memcpy(image+build->cap4-19,cap4::signature.data(),cap4::signature.size());
        auto sig=spread::signature;std::memcpy(sig.data()+43,&build->sweep_call_displacement,4);
        std::memcpy(image+build->spread-27,sig.data(),sig.size());
        check(cap4::validate_image(image,size,build),tag+"CAP4_identity");
        check(spread::validate_image(image,size,build),tag+"spread_identity");
        const auto* other=build==&fix_builds::ets157?&fix_builds::ets158:&fix_builds::ets157;
        check(!cap4::validate_image(image,size,other)&&!spread::validate_image(image,size,other),tag+"cross_build_rejected");
        check(!cap4::validate_image(image,size,nullptr)&&!spread::validate_image(image,size,nullptr),tag+"null_descriptor_rejected");
        image[build->cap4]=0x72;
        check(!cap4::validate_image(image,size,build),tag+"CAP4_changed_byte_rejected");image[build->cap4]=0x73;
        image[build->spread+3]^=1;
        check(!spread::validate_image(image,size,build),tag+"spread_changed_byte_rejected");image[build->spread+3]^=1;
        cap4::Site cap(image+build->cap4,{reinterpret_cast<std::uintptr_t>(image)+build->generator_begin,build->generator_end-build->generator_begin});
        spread::Site sweep(image+build->spread,{reinterpret_cast<std::uintptr_t>(image)+build->sweep_begin,build->sweep_end-build->sweep_begin});
        lifecycle::Controller c,p;
        check(c.install(true,cap)&&image[build->cap4]==0x90&&image[build->cap4+1]==0x90,tag+"CAP4_install");
        check(p.install(true,sweep)&&image[build->spread]==0xe8&&image[build->spread+5]==0xe3&&image[build->spread+6]==0x16,tag+"spread_install_original_branches");
        check(p.stop(sweep)&&std::memcmp(image+build->spread,spread::original.data(),9)==0,tag+"spread_restore");
        check(c.stop(cap)&&image[build->cap4]==0x73&&image[build->cap4+1]==0x0a,tag+"CAP4_restore");
        check(cap4::validate_image(image,size,build)&&spread::validate_image(image,size,build),tag+"restored_identity");
    }
    VirtualFree(image,0,MEM_RELEASE);
    std::printf("PORT_TOTAL %u\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL PORT %s\n",e.what());return 1;}}
