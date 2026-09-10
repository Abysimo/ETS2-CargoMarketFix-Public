#include "cap4_patch.h"
#include "refresh_spread.h"
#include "logger.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

static unsigned cases;
extern "C" std::uint64_t cap159_fixture(std::uint64_t);
extern "C" unsigned char cap159_site,cap159_end;
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
    constexpr std::size_t size=0x7b0000;
    auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!image)throw std::runtime_error("allocation");
    for(const auto* build:fix_builds::supported){
        const auto tag=std::string(build->version)+"_";
        std::memset(image,0,size);
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+0x100);nt->Signature=IMAGE_NT_SIGNATURE;
        nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt->FileHeader.NumberOfSections=1;
        nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
        nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=size;
        auto* s=IMAGE_FIRST_SECTION(nt);std::memcpy(s->Name,".text",5);
        s->VirtualAddress=0x1000;s->Misc.VirtualSize=size-0x1000;s->Characteristics=IMAGE_SCN_MEM_EXECUTE;
        std::memcpy(image+build->cap4-19,build->cap4_signature,build->cap4_signature_size);
        auto spec=build->spread_signature;
        if(!spec.bytes)spec={spread::signature.data(),spread::signature.size(),27,43};
        std::array<std::uint8_t,256> sig{};std::memcpy(sig.data(),spec.bytes,spec.size);
        std::memcpy(sig.data()+spec.call_offset,&build->sweep_call_displacement,4);
        std::memcpy(image+build->spread-spec.patch_offset,sig.data(),spec.size);
        const auto& layout=*spread::layout_for(build->spread_strategy);
        check(cap4::validate_image(image,size,build),tag+"CAP4_identity");
        check(spread::validate_image(image,size,build),tag+"spread_identity");
        bool isolated=true;
        for(const auto* other:fix_builds::supported)if(other!=build)
            isolated=isolated&&!cap4::validate_image(image,size,other)&&!spread::validate_image(image,size,other);
        check(isolated,tag+"cross_build_rejected");
        check(!cap4::validate_image(image,size,nullptr)&&!spread::validate_image(image,size,nullptr),tag+"null_descriptor_rejected");
        image[build->cap4]=0x72;
        check(!cap4::validate_image(image,size,build),tag+"CAP4_changed_byte_rejected");image[build->cap4]=0x73;
        image[build->spread+3]^=1;
        check(!spread::validate_image(image,size,build),tag+"spread_changed_byte_rejected");image[build->spread+3]^=1;
        cap4::Site cap(image+build->cap4,{reinterpret_cast<std::uintptr_t>(image)+build->generator_begin,build->generator_end-build->generator_begin},build->cap4_original,build->cap4_write_strategy);
        spread::Site sweep(image+build->spread,{reinterpret_cast<std::uintptr_t>(image)+build->sweep_begin,build->sweep_end-build->sweep_begin},layout);
        lifecycle::Controller c,p;
        check(c.install(true,cap)&&image[build->cap4]==0x90&&image[build->cap4+1]==0x90,tag+"CAP4_install");
        check(p.install(true,sweep)&&image[build->spread]==0xe8&&image[build->spread+5]==0xe3&&image[build->spread+6]==layout.empty_displacement,tag+"spread_install_original_branches");
        check(p.stop(sweep)&&std::memcmp(image+build->spread,layout.original.data(),layout.span)==0,tag+"spread_restore");
        check(c.stop(cap)&&image[build->cap4]==0x73&&image[build->cap4+1]==(build->cap4_original>>8),tag+"CAP4_restore");
        check(cap4::validate_image(image,size,build)&&spread::validate_image(image,size,build),tag+"restored_identity");
    }
    VirtualFree(image,0,MEM_RELEASE);
    check(fix_builds::identify(fix_builds::ets159.sha256)==&fix_builds::ets159,"159_exact_identity");
    bool canonical=true;
    for(const auto* b:fix_builds::supported)canonical=canonical&&fix_builds::known(b)&&fix_builds::identify(b->sha256)==b;
    check(canonical&&!fix_builds::known(nullptr),"all_builds_canonical");
    check(!fix_builds::identify("1.59.1.3")&&!fix_builds::identify("1.59.x"),"159_no_version_wildcard");
    check(fix_builds::ets157.cap4==0x6ce618&&fix_builds::ets158.cap4==0x6f4ce8&&
        fix_builds::ets157.cap4_original==0x0a73&&fix_builds::ets158.cap4_original==0x0a73,"legacy_targets_bytes_retained");
    const auto begin=reinterpret_cast<std::uintptr_t>(&cap159_fixture);
    cap4::Site native(&cap159_site,{begin,reinterpret_cast<std::uintptr_t>(&cap159_end)-begin},fix_builds::ets159.cap4_original);
    check((&cap159_site)[0]==0x73&&(&cap159_site)[1]==0x0b&&
        std::memcmp(&cap159_site-19,fix_builds::ets159.cap4_signature,64)==0,"159_native_encoding_and_local_stack_budget");
    const std::uint64_t inputs[]{0,1,3,4,5,9,10,11,100,10000,UINT64_MAX};
    bool original=true;
    for(auto n:inputs)original=original&&cap159_fixture(n)==(n<4?4:(n>10?10:n));
    check(original,"159_native_original_floor_four_ceiling_ten");
    lifecycle::Controller owner;
    check(owner.install(true,native),"159_native_atomic_install");
    bool fixed=true;for(auto n:inputs)fixed=fixed&&cap159_fixture(n)==4;
    check(fixed,"159_native_fixed_four_all_admitted_counts");
    check(owner.stop(native)&&(&cap159_site)[0]==0x73&&(&cap159_site)[1]==0x0b,"159_native_exact_restore");
    original=true;for(auto n:inputs)original=original&&cap159_fixture(n)==(n<4?4:(n>10?10:n));
    check(original,"159_native_restored_budget_semantics");
    check(fix_builds::supported.size()==4&&fix_builds::supported[0]==&fix_builds::ets157&&
        fix_builds::supported[1]==&fix_builds::ets158&&fix_builds::supported[2]==&fix_builds::ets159&&
        fix_builds::supported[3]==&fix_builds::ets160,
        "exact_release_supported_set");
    cap4::Site wrong_original(&cap159_site,{begin,reinterpret_cast<std::uintptr_t>(&cap159_end)-begin},fix_builds::ets158.cap4_original);
    lifecycle::Controller wrong_owner;
    check(!wrong_owner.install(true,wrong_original)&&!wrong_owner.may_be_live()&&
        (&cap159_site)[0]==0x73&&(&cap159_site)[1]==0x0b,"legacy_original_cannot_own_159_branch");
    std::printf("PORT_TOTAL %u\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL PORT %s\n",e.what());return 1;}}
