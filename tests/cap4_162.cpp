#include "cap4_patch.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

extern "C" std::uint64_t cap162_fixture(std::uint64_t);
extern "C" unsigned char cap162_site,cap162_end;
static unsigned cases=0;
static void check(bool ok,const char* name) {
    if(!ok)throw std::runtime_error(name);
    ++cases;std::printf("PASS C162_%s\n",name);
}
int main(){try {
    using namespace cmf;
    const auto& build=fix_builds::ets1622;
    check(fix_builds::identify(build.sha256)==&build&&fix_builds::known(&build)&&
        !fix_builds::identify("1.62.2.1"),"exact_descriptor_no_version_wildcard");
    check(!(build.cap4&1)&&!(reinterpret_cast<std::uintptr_t>(&cap162_site)&1)&&
        build.cap4_write_strategy==fix_builds::Cap4WriteStrategy::quiesced_copy2,
        "aligned_target_retains_quiesced_copy2");
    check(build.cap4_signature_size==70&&build.cap4_signature==fix_builds::ets162.cap4_signature&&
        !std::memcmp(&cap162_site-19,build.cap4_signature,70),"exact_70_byte_shared_162_clamp");
    check(std::memcmp(&cap162_site-19,fix_builds::ets1619.cap4_signature,64)!=0,
        "old_stack_signature_is_not_reused");
    const std::uint64_t inputs[]{0,1,3,4,5,9,10,11,100,10000,UINT64_MAX};
    bool original=true;for(auto n:inputs)original=original&&cap162_fixture(n)==(n<4?4:(n>10?10:n));
    check(original,"native_original_attempt_counter_floor_count_ceiling");
    const auto begin=reinterpret_cast<std::uintptr_t>(&cap162_fixture);
    cap4::Site site(&cap162_site,{begin,reinterpret_cast<std::uintptr_t>(&cap162_end)-begin},
        build.cap4_original,build.cap4_write_strategy);
    const auto refused=site.patch(true);
    check(!refused.committed&&!refused.write_attempted&&cap162_site==0x73,
        "even_site_still_rejects_unquiesced_write");
    const auto left=(&cap162_site)[-1],right=(&cap162_site)[2];
    lifecycle::Controller owner;
    check(owner.install(true,site)&&owner.module_pinned()&&cap162_site==0x90&&
        (&cap162_site)[1]==0x90,"production_install_PIN_two_nops");
    bool four=true;for(auto n:inputs)four=four&&cap162_fixture(n)==4;
    check(four,"native_attempt_counter_exactly_four_for_all_inputs");
    check((&cap162_site)[-1]==left&&(&cap162_site)[2]==right,"patch_neighbors_preserved");
    check(owner.stop(site)&&owner.release_allowed()&&
        !std::memcmp(&cap162_site-19,build.cap4_signature,70),"full_clamp_exact_restore");
    original=true;for(auto n:inputs)original=original&&cap162_fixture(n)==(n<4?4:(n>10?10:n));
    check(original,"restored_native_attempt_counter");
    check(owner.stop(site)&&(&cap162_site)[-1]==left&&(&cap162_site)[2]==right,
        "idempotent_close_and_clean_neighbors");
    std::printf("CAP4_162_TOTAL %u\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL CAP4_162 %s\n",e.what());return 1;}}
