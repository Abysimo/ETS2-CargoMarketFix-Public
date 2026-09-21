#include "refresh_spread.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <algorithm>
using namespace cmf;
struct Economy {unsigned char pad[0x19c]{};std::uint32_t minute=0;};
struct Company {unsigned visits=0;};
extern "C" {
void s160_fixture(Company**,std::uint64_t,Economy*),s160_probe();
extern unsigned char s160_site,s160_loop,s160_finish,s160_end;
std::uint64_t s160_input[4]{},s160_regs[15]{},s160_rsp_before{},s160_rsp_after{};
alignas(16) unsigned char s160_fx_before[512]{},s160_fx_after[512]{};
}
static unsigned cases=0,mode=0,visits=0,searches=0,cleanup=0;
static void need(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
static void check(bool ok,const char* s){need(ok,s);++cases;std::printf("PASS S161_%s\n",s);}
extern "C" void s160_visit(Company* c,Economy*) {
    if(mode==1)throw 161;
    if(mode==2)RaiseException(0xe1610060,0,0,nullptr);
    ++visits;++c->visits;
}
static MemoryRange range(){auto b=reinterpret_cast<std::uintptr_t>(&s160_fixture);return {b,reinterpret_cast<std::uintptr_t>(&s160_end)-b};}
static void probe(std::uint64_t n,std::uint64_t b,std::uint64_t e,Economy* eco){
    s160_input[0]=n;s160_input[1]=b;s160_input[2]=e;s160_input[3]=reinterpret_cast<std::uintptr_t>(eco);s160_probe();
}
static LONG CALLBACK search(EXCEPTION_POINTERS* p){if(p->ExceptionRecord->ExceptionCode==0xe1610060){++searches;return EXCEPTION_CONTINUE_EXECUTION;}return EXCEPTION_CONTINUE_SEARCH;}
static void fault(Company** p){__try {__try {s160_fixture(p,1440,nullptr);}__finally {++cleanup;}}
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}}
static DWORD WINAPI idle(void*){return 0;}
static bool blocks(spread::Site& site,std::uintptr_t ip){
    auto h=CreateThread(nullptr,0,idle,nullptr,CREATE_SUSPENDED,nullptr);need(h!=nullptr,"thread");
    CONTEXT old{};old.ContextFlags=CONTEXT_CONTROL;need(GetThreadContext(h,&old)!=0,"context");
    auto now=old;now.Rip=ip;need(SetThreadContext(h,&now)!=0,"set IP");
    auto ok=site.freeze();if(ok)site.thaw();
    need(SetThreadContext(h,&old)&&ResumeThread(h)!=DWORD(-1),"restore worker");
    need(WaitForSingleObject(h,10000)==WAIT_OBJECT_0,"join");CloseHandle(h);return !ok;
}
#include "spread_v160_faults.h"
int main(){try{
    check(&s160_loop-&s160_site==9&&&s160_finish-(&s160_site+7)==0x16,"nine_byte_whole_CMP_JE_NOP");
    unsigned char saved[11];std::memcpy(saved,&s160_site-1,11);
    check(!std::memcmp(&s160_site,spread::v161_layout.original.data(),9),"exact_original");
    spread::Site wrong(&s160_site,range(),spread::v160_layout);
    check(!wrong.prepare(),"v160_cannot_own_161");
    spread::Site site(&s160_site,range(),spread::v161_layout,1440);lifecycle::Controller ctl;
    check(ctl.install(true,site)&&ctl.module_pinned(),"install_PIN");
    check(s160_site==0xe8&&*(&s160_site+5)==0xe3&&*(&s160_site+6)==0x16,"CALL_JRCXZ");
    check(blocks(site,reinterpret_cast<std::uintptr_t>(&s160_loop)),"quiescence_caller");
    check(blocks(site,site.relay_range().begin),"quiescence_relay");
    check(blocks(site,site.bridge_range().begin),"quiescence_bridge");
    for(unsigned n:{0u,1u,1439u,1440u,1441u,12921u}){
        std::vector<Company> c(n);std::vector<Company*> p;for(auto& x:c)p.push_back(&x);
        for(unsigned m=0;m<1440;++m){Economy e;e.minute=m;auto before=visits;
            s160_fixture(p.data(),n,&e);
            auto first=m*(n/1440)+std::min(m,n%1440),length=n/1440+(m<n%1440);
            need(visits-before==length,"slice length");
            for(unsigned i=0;i<n;++i)need(c[i].visits==unsigned(i<first+length),"ordered exactly once");}
        check(true,("partition_"+std::to_string(n)).c_str());
    }
    Economy e;e.minute=7;probe(1440,0x10000,0x12d00,&e);
    check(s160_regs[0]==0x10000&&s160_regs[1]==1&&s160_regs[2]==0x12345678&&s160_regs[3]==0x10038&&
        s160_regs[4]==reinterpret_cast<std::uintptr_t>(&e)&&s160_regs[5]==0x11223344&&s160_regs[6]==0x10040&&
        s160_regs[7]==0x123456789&&s160_regs[8]==0x987654321&&s160_regs[9]==0x456789012&&
        s160_regs[10]==0x567890123&&s160_regs[11]==0x12121212&&s160_regs[12]==0x13131313&&
        s160_regs[13]==0x14141414&&s160_regs[14]==0x15151515,"GPR_ABI");
    check(s160_rsp_before==s160_rsp_after&&!std::memcmp(s160_fx_before,s160_fx_after,512),"RSP_XMM_FP_MXCSR");
    for(auto m:{0u,1439u,1440u,UINT32_MAX}){e.minute=m;probe(1440,0x10000,0x12d00,&e);
        need(s160_regs[3]==0x10000+8ull*(m%1440)&&s160_regs[6]==s160_regs[3]+8,"unsigned minute");}
    check(true,"absolute_unsigned_minutes");
    probe(UINT64_MAX,0x10000,0x10000,nullptr);
    check(s160_regs[1]==0&&s160_regs[3]==s160_regs[6]&&cmf_refresh_spread_active==0,"original_empty_precedes_N_and_minute_read");
    for(auto n:{0ull,1ull<<61,UINT64_MAX}){probe(n,0x10000,0x10008,&e);
        need(s160_regs[1]==1&&s160_regs[3]==0x10000&&s160_regs[6]==0x10008,"invalid full range");}
    check(true,"invalid_N_full_range");
    probe(2,UINT64_MAX-7,8,&e);check(s160_regs[1]==1&&s160_regs[3]==UINT64_MAX-7&&s160_regs[6]==8,"pointer_overflow_full_range");
    probe(1440,0x10000,0x10008,&e);check(s160_regs[1]==1&&s160_regs[3]==0x10000&&s160_regs[6]==0x10008,"end_mismatch_full_range");
    e.minute=1439;probe(1,0x10000,0x10008,&e);check(s160_regs[1]==0&&s160_regs[3]==s160_regs[6],"empty_bucket");
    std::vector<Company> c(1440);std::vector<Company*> p;for(auto& x:c)p.push_back(&x);
    mode=1;bool caught=false;try{s160_fixture(p.data(),1440,&e);}catch(int x){caught=x==161;}
    check(caught&&cmf_refresh_spread_active==0,"CXX_unwind");
    mode=2;auto h=AddVectoredExceptionHandler(1,search);need(h!=nullptr,"VEH");auto before=visits;
    s160_fixture(p.data(),1440,&e);RemoveVectoredExceptionHandler(h);mode=0;
    check(searches==1&&visits==before+1&&cmf_refresh_spread_active==0,"SEH_continue_execution");
    check(ctl.stop(site)&&!site.has_allocation()&&!std::memcmp(saved,&s160_site-1,11),"exact_restore_neighbors");
    before=visits;s160_fixture(p.data(),1440,&e);check(visits==before+1440,"restored_full_range");
    for(auto f:{Inject::prepare,Inject::writable,Inject::write,Inject::flush,Inject::protection}){
        SpreadFault ops(&s160_site,range(),f,spread::v161_layout);lifecycle::Controller owner;
        need(!owner.install(true,ops)&&!ops.bad_order&&!ops.frozen&&!ops.has_allocation()&&
             !std::memcmp(saved,&s160_site-1,11),"rollback");
        check(true,"partial_failure_rollback");}
    {SpreadFault ops(&s160_site,range(),Inject::restore,spread::v161_layout);lifecycle::Controller owner;
        need(owner.install(true,ops),"restore setup");
        check(!owner.stop(ops)&&owner.phase()==lifecycle::Phase::containment&&ops.has_allocation(),"uncertain_restore_containment");
        check(owner.stop(ops)&&!ops.has_allocation()&&!ops.bad_order,"fresh_quiesced_restore");}
    spread::Site fault_site(&s160_site,range(),spread::v161_layout,1440);lifecycle::Controller fc;
    need(fc.install(true,fault_site),"fault install");fault(p.data());
    check(cleanup==1&&cmf_refresh_spread_active==1,"bridge_fault_unwind_retains_lease");
    check(!fc.stop(fault_site)&&fc.phase()==lifecycle::Phase::containment&&fault_site.has_allocation(),"fault_contains_callable_resources");
    std::printf("SPREAD161_TOTAL %u\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL SPREAD161 %s\n",e.what());return 1;}}
