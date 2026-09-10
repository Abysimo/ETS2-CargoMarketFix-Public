#include "refresh_spread.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <stdexcept>
using namespace cmf;
struct Economy {unsigned char pad[0x19c]{};std::uint32_t minute=0;};
struct Company {unsigned visits=0;};
extern "C" {
void s160_fixture(Company**,std::uint64_t,Economy*),s160_probe();
void spread_fixture(Company**,std::uint64_t,Economy*);
extern unsigned char spread_site,spread_fixture_end;
std::uint64_t spread_registers[10]{},spread_rsp_before{},spread_rsp_after{};
alignas(16) unsigned char spread_fx_before[512]{},spread_fx_after[512]{};
extern unsigned char s160_site,s160_loop,s160_finish,s160_end;
std::uint64_t s160_input[4]{},s160_regs[15]{},s160_rsp_before{},s160_rsp_after{};
alignas(16) unsigned char s160_fx_before[512]{},s160_fx_after[512]{};
}
static unsigned count=0,mode=0,visits=0,searches=0,cleanup=0;
static void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static void check(bool b,const char* s){need(b,s);std::printf("PASS S160_%02u_%s\n",++count,s);}
extern "C" void s160_visit(Company* c,Economy*){
    if(mode==1)throw 160;
    if(mode==2)RaiseException(0xe1600060,0,0,nullptr);
    ++visits;++c->visits;
}
extern "C" void spread_test_visit(Company* c,Economy* e){s160_visit(c,e);}
static MemoryRange range(){auto b=reinterpret_cast<std::uintptr_t>(&s160_fixture);return {b,reinterpret_cast<std::uintptr_t>(&s160_end)-b};}
static LONG CALLBACK search(EXCEPTION_POINTERS* p){if(p->ExceptionRecord->ExceptionCode==0xe1600060){++searches;return EXCEPTION_CONTINUE_EXECUTION;}return EXCEPTION_CONTINUE_SEARCH;}
static void fault(Company** p){
    __try {__try {s160_fixture(p,60,nullptr);}__finally {++cleanup;}}
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}
}
static DWORD WINAPI idle(void*){return 0;}
static bool blocks(spread::Site& site,std::uintptr_t ip){
    HANDLE h=CreateThread(nullptr,0,idle,nullptr,CREATE_SUSPENDED,nullptr);need(h!=nullptr,"worker");
    CONTEXT old{};old.ContextFlags=CONTEXT_CONTROL;need(GetThreadContext(h,&old)!=0,"context");
    auto now=old;now.Rip=ip;need(SetThreadContext(h,&now)!=0,"IP");bool acquired=site.freeze();if(acquired)site.thaw();
    need(SetThreadContext(h,&old)!=0&&ResumeThread(h)!=DWORD(-1),"resume");need(WaitForSingleObject(h,10000)==WAIT_OBJECT_0,"join");CloseHandle(h);return !acquired;
}
static void cycle(std::size_t n,void(*sweep)(Company**,std::uint64_t,Economy*)=s160_fixture){
    std::vector<Company> c(n);std::vector<Company*> p;for(auto& x:c)p.push_back(&x);
    std::size_t end=0,small=n,big=0;
    for(unsigned m=0;m<60;++m){Economy e;e.minute=m;std::memcpy(e.pad+0x15c,&m,4);auto before=visits;sweep(p.data(),n,&e);
        auto length=n/60+(m<n%60);need(visits-before==length,"bucket length");end+=length;
        for(std::size_t i=0;i<n;++i)need(c[i].visits==(i<end?1u:0u),"exact contiguous once coverage");
        small=std::min(small,length);big=std::max(big,length);
    }need(end==n&&big-small<=1,"balanced complete");
}
static void probe(std::uint64_t n,std::uint64_t begin,std::uint64_t end,Economy* e){
    s160_input[0]=n;s160_input[1]=begin;s160_input[2]=end;s160_input[3]=reinterpret_cast<std::uintptr_t>(e);s160_probe();
}
#include "spread_v160_faults.h"
static void signatures(){
    constexpr std::size_t size=0x4000;auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));need(image!=nullptr,"image");
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+0x100);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt->FileHeader.NumberOfSections=1;nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=size;
    auto* section=IMAGE_FIRST_SECTION(nt);std::memcpy(section->Name,".text",5);section->VirtualAddress=0x1000;section->Misc.VirtualSize=0x3000;section->Characteristics=IMAGE_SCN_MEM_EXECUTE;
    // Authored signature includes sentinel and complete post-padding MOV.
    const unsigned char sig[]{0x42,0x0f,0x1f,0x84,0,0,0,0,0,0x48,0x8b,0x0b};std::memcpy(image+0x1100,sig,sizeof(sig));
    fix_builds::SpreadSignature spec{sig,sizeof(sig),1,SIZE_MAX};
    auto valid=[&](const spread::Layout& l,fix_builds::SpreadSignature s){return spread::validate_site_image(image,size,0x1101,0x1000,0x2000,l,s,0);};
    check(valid(spread::v160_layout,spec),"exact_authored_signature_metadata");
    check(!valid(spread::legacy_layout,spec),"signature_cross_ABI_refused");
    std::memcpy(image+0x1200,sig,sizeof(sig));check(!valid(spread::v160_layout,spec),"duplicate_signature_refused");std::memset(image+0x1200,0,sizeof(sig));
    image[0x1105]^=1;check(!valid(spread::v160_layout,spec),"signature_corruption_refused");image[0x1105]^=1;
    auto bad=spec;bad.patch_offset=2;check(!valid(spread::v160_layout,bad),"wrong_signature_patch_offset_refused");
    bad=spec;bad.call_offset=sizeof(sig)-2;check(!valid(spread::v160_layout,bad),"bad_call_substitution_bounds_refused");
    check(!spread::validate_site_image(image,size,0x1101,0x1101,0x1108,spread::v160_layout,spec,0),"incomplete_owned_range_refused");
    auto foreign=fix_builds::ets159;foreign.spread_strategy=spread::Strategy::v160_rcx_rbp;foreign.spread_signature=spec;
    check(!spread::validate_image(image,size,&foreign),"foreign_descriptor_cannot_enable_v160");VirtualFree(image,0,MEM_RELEASE);
}
int main(){std::setvbuf(stdout,nullptr,_IONBF,0);try{
    check(&s160_loop-&s160_site==8&&&s160_finish-(&s160_site+7)==0x15,"exact_eight_byte_and_empty_layout");
    unsigned char before[12];std::memcpy(before,&s160_site-1,sizeof(before));
    check(std::memcmp(&s160_site,spread::v160_layout.original.data(),8)==0,"original_eight_bytes");
    spread::Site legacy(&s160_site,range());check(!legacy.prepare(),"legacy_rejects_v160_padding");
    auto bad=spread::v160_layout;bad.empty_displacement=0x16;spread::Site wrong(&s160_site,range(),bad);
    check(!wrong.prepare()&&!wrong.has_allocation(),"wrong_empty_metadata_refused");
    bad=spread::v160_layout;bad.span=9;spread::Site wide(&s160_site,range(),bad);check(!wide.prepare(),"widened_span_refused");
    for(auto* b:{&fix_builds::ets157,&fix_builds::ets158,&fix_builds::ets159})check(b->spread_strategy==spread::Strategy::legacy_v157_v159,"supported_build_remains_legacy");
    std::vector<Company> c(60);std::vector<Company*> p;for(auto& x:c)p.push_back(&x);Economy e;e.minute=7;
    s160_fixture(p.data(),60,&e);check(visits==60,"unpatched_full_traversal");
    spread::Site site(&s160_site,range(),spread::v160_layout);lifecycle::Controller owner;
    check(owner.install(true,site)&&owner.module_pinned(),"native_install_PIN");
    check(site.bridge_range().begin==reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge_v160)&&
        site.bridge_range().begin!=reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge),"dedicated_bridge_range");
    std::int32_t rel=0;std::memcpy(&rel,&s160_site+1,4);auto relay=site.relay_range().begin;
    std::uintptr_t dest=0;std::memcpy(&dest,reinterpret_cast<void*>(relay+6),8);
    MEMORY_BASIC_INFORMATION mi{};VirtualQuery(reinterpret_cast<void*>(relay),&mi,sizeof(mi));
    check(s160_site==0xe8&&*(&s160_site+5)==0xe3&&*(&s160_site+6)==0x15&&*(&s160_site+7)==0x90&&
        reinterpret_cast<std::uintptr_t>(&s160_site+5)+rel==relay&&
        *reinterpret_cast<unsigned char*>(relay)==0xff&&*reinterpret_cast<unsigned char*>(relay+1)==0x25&&
        dest==site.bridge_range().begin&&mi.Protect==PAGE_EXECUTE_READ,"rel32_relay_selected_bridge_RX");
    check(*(&s160_site-1)==before[0]&&!std::memcmp(&s160_site+8,before+9,3),"neighbor_and_following_MOV_untouched");
    check(blocks(site,reinterpret_cast<std::uintptr_t>(&s160_loop)),"quiescence_game");
    check(blocks(site,relay),"quiescence_relay");check(blocks(site,site.bridge_range().begin),"quiescence_selected_bridge");
    for(auto n:{1u,2u,4u,59u,60u,61u,119u,120u,121u,12921u}){cycle(n);check(true,"sixty_native_buckets_exact_balanced");}
    cycle(0);check(true,"original_empty_scope_bypasses_patch");
    for(auto minute:{60u,61u,119u,120u,1000000u,UINT32_MAX}){e.minute=minute;auto v=visits;s160_fixture(p.data(),60,&e);need(visits==v+1,"large minute traversal");
        probe(60,0x10000,0x101e0,&e);need(s160_regs[3]==0x10000+8ull*(minute%60)&&s160_regs[6]==s160_regs[3]+8,"large minute modulo");}
    check(true,"large_absolute_minutes_modulo");e.minute=7;probe(60,0x10000,0x101e0,&e);
    check(s160_regs[0]==0x10000&&s160_regs[1]==1&&s160_regs[2]==0x12345678&&s160_regs[3]==0x10038&&s160_regs[6]==0x10040&&
        s160_regs[4]==reinterpret_cast<std::uintptr_t>(&e)&&s160_regs[5]==0x11223344&&s160_regs[7]==0x123456789&&s160_regs[8]==0x987654321&&
        s160_regs[9]==0x456789012&&s160_regs[10]==0x567890123&&s160_regs[11]==0x12121212&&s160_regs[12]==0x13131313&&s160_regs[13]==0x14141414&&s160_regs[14]==0x15151515,"all_GPR_inputs_outputs_preserved");
    check(s160_rsp_before==s160_rsp_after&&!std::memcmp(s160_fx_before,s160_fx_after,512),"RSP_XMM_x87_MXCSR_preserved");
    for(auto n:{0ull,1ull<<61,UINT64_MAX}){probe(n,0x10000,0x101e0,&e);need(s160_regs[1]==1&&s160_regs[3]==0x10000&&s160_regs[6]==0x101e0,"invalid N full range");}
    check(true,"zero_and_multiplication_overflow_fail_open");
    probe(2,UINT64_MAX-7,8,&e);check(s160_regs[1]==1&&s160_regs[3]==UINT64_MAX-7&&s160_regs[6]==8,"addition_overflow_fail_open");
    probe(60,0x10000,0x10008,&e);check(s160_regs[1]==1&&s160_regs[3]==0x10000&&s160_regs[6]==0x10008,"end_mismatch_fail_open");
    e.minute=59;probe(1,0x10000,0x10008,&e);check(s160_regs[1]==0&&s160_regs[3]==s160_regs[6],"empty_bucket_RCX_zero");
    mode=1;bool caught=false;try{s160_fixture(p.data(),60,&e);}catch(int x){caught=x==160;}
    check(caught&&cmf_refresh_spread_active==0,"original_CXX_unwind");
    mode=2;auto handler=AddVectoredExceptionHandler(1,search);need(handler!=nullptr,"VEH");auto v=visits;s160_fixture(p.data(),60,&e);RemoveVectoredExceptionHandler(handler);mode=0;
    check(searches==1&&visits==v+1&&cmf_refresh_spread_active==0,"continue_execution_once");
    check(owner.stop(site)&&!site.has_allocation()&&!std::memcmp(&s160_site-1,before,sizeof(before)),"exact_eight_restore_relay_release");
    for(unsigned i=0;i<100;++i){spread::Site s(&s160_site,range(),spread::v160_layout);lifecycle::Controller o;need(o.install(true,s),"repeat install");
        auto first=*(&s160_site-1),next=*(&s160_site+8);s160_fixture(p.data(),60,&e);need(first==before[0]&&next==before[9]&&o.stop(s)&&!s.has_allocation()&&!std::memcmp(&s160_site-1,before,12),"repeat boundary restore");}
    check(true,"100_native_boundary_cycles");
    {const auto b=reinterpret_cast<std::uintptr_t>(&spread_fixture);MemoryRange lr{b,reinterpret_cast<std::uintptr_t>(&spread_fixture_end)-b};
        spread::Site cross(&spread_site,lr,spread::v160_layout);check(!cross.prepare(),"v160_rejects_legacy_padding");
        spread::Site ls(&spread_site,lr);lifecycle::Controller lc;need(lc.install(true,ls),"legacy install");
        check(ls.bridge_range().begin==reinterpret_cast<std::uintptr_t>(&cmf_refresh_spread_bridge)&&*(&spread_site+6)==0x16,"legacy_bridge_and_empty_displacement");
        for(auto n:{1u,2u,4u,59u,60u,61u,119u,120u,121u,12921u})cycle(n,spread_fixture);
        check(true,"legacy_native_full_60_bucket_regression");
        check(lc.stop(ls)&&!std::memcmp(&spread_site,spread::original.data(),9)&&!ls.has_allocation(),"legacy_exact_nine_byte_restore");}
    e.minute=7;unsigned wrongMinute=19;std::memcpy(e.pad+0x15c,&wrongMinute,4);probe(60,0x10000,0x101e0,&e);
    check(s160_regs[3]==0x10038&&s160_regs[3]!=0x10000+8ull*wrongMinute,"minute_ABI_negative_control");
    probe(59,0x10000,0x101e0,&e);check(s160_regs[3]==0x10000&&s160_regs[1]==1,"wrong_count_ABI_negative_control");
    signatures();
    check(fix_builds::supported.size()==4&&fix_builds::identify("B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8")==&fix_builds::ets160,"real_160_exact_recognized");
    for(auto f:{Inject::prepare,Inject::writable,Inject::write,Inject::flush,Inject::protection}){
        SpreadFault ops(&s160_site,range(),f);lifecycle::Controller ctl;
        need(!ctl.install(true,ops),"fault unexpectedly installed");
        need(!ops.bad_order&&!ops.frozen&&!std::memcmp(&s160_site-1,before,12)&&!ops.has_allocation(),"fault exact rollback cleanup");
        if(f==Inject::flush||f==Inject::protection)need(ops.writes==2&&ctl.last_patch().rollback_attempted,"rollback under freeze");
        check(true,"fault_injection_install_cleanup");
    }
    {SpreadFault ops(&s160_site,range(),Inject::restore);lifecycle::Controller ctl;need(ctl.install(true,ops),"restore setup");
        check(!ctl.stop(ops)&&ctl.phase()==lifecycle::Phase::containment&&ops.has_allocation()&&!ops.frozen,"restoration_failure_contains_relay");
        check(ctl.stop(ops)&&!ops.bad_order&&!ops.has_allocation()&&!std::memcmp(&s160_site-1,before,12),"fresh_frozen_restoration_retry");
        const auto writes=ops.writes;auto repaired=ops.repair_original_auxiliaries();
        check(repaired.committed&&ops.writes==writes,"post_detach_auxiliary_zero_writes");}
    // A bridge memory fault intentionally leaves its lease/resources contained.
    spread::Site fault_site(&s160_site,range(),spread::v160_layout);lifecycle::Controller fc;need(fc.install(true,fault_site),"fault install");fault(p.data());
    check(cleanup==1&&cmf_refresh_spread_active==1,"bridge_SEH_unwind_retains_lease");
    check(!fc.stop(fault_site)&&fc.may_be_live()&&fault_site.has_allocation()&&fc.phase()==lifecycle::Phase::containment,"fault_containment_retains_callable_relay");
    std::printf("SPREAD160_TOTAL %u\n",count);return 0;
}catch(const std::exception& e){std::printf("FAIL SPREAD160 %s\n",e.what());return 1;}}
