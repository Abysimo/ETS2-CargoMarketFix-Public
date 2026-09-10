#include "cap4_patch.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <stdexcept>
using namespace cmf;
using namespace cmf::lifecycle;
using Strategy=cap4::WriteStrategy;
static unsigned cases=0;
static void need(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
static void check(bool ok,const char* what){need(ok,what);std::printf("PASS Q%02u_%s\n",++cases,what);}
struct Fixture {
    unsigned char* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Fixture(){need(base!=nullptr,"fixture allocation");
        // Authored function: original returns 4 below four, 10 otherwise.
        // Offset 9 is a real odd-address JAE +11; removing it takes floor four.
        const unsigned char code[]{0xb8,4,0,0,0,0x83,0xf9,4,0x90,0x73,0x0b,
            0xb8,4,0,0,0,0xc3,0x90,0x90,0x90,0x90,0x90,0xb8,10,0,0,0,0xc3};
        std::memcpy(base,code,sizeof(code));
        // Real worker announces entry then spins entirely inside this image.
        const unsigned char loop[]{0xc6,1,1,0x80,0x79,1,0,0x75,4,0xf3,0x90,0xeb,0xf6,0x31,0xc0,0xc3};
        std::memcpy(base+128,loop,sizeof(loop));
        DWORD old=0;need(VirtualProtect(base,8192,PAGE_EXECUTE_READ,&old)!=0,"fixture RX");
        need(FlushInstructionCache(GetCurrentProcess(),base,8192)!=0,"fixture cache");
    }
    ~Fixture(){VirtualFree(base,0,MEM_RELEASE);}
    unsigned char* target()const{return base+9;}
    MemoryRange range()const{return {reinterpret_cast<std::uintptr_t>(base),4096};}
    bool neighbors()const{return base[8]==0x90&&base[11]==0xb8;}
    bool original()const{return target()[0]==0x73&&target()[1]==0x0b&&neighbors();}
    bool patched()const{return target()[0]==0x90&&target()[1]==0x90&&neighbors();}
    unsigned run(unsigned n)const{return reinterpret_cast<unsigned(*)(unsigned)>(base)(n);}
    DWORD protection()const{MEMORY_BASIC_INFORMATION m{};need(VirtualQuery(base,&m,sizeof(m))!=0,"query");return m.Protect;}
};
struct Inside {
    volatile unsigned char flags[2]{0,0}; HANDLE thread=nullptr;
    explicit Inside(Fixture& f){thread=CreateThread(nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(f.base+128),const_cast<unsigned char*>(flags),0,nullptr);need(thread!=nullptr,"inside thread");
        const auto end=GetTickCount64()+5000;while(!flags[0]&&GetTickCount64()<end)Sleep(1);need(flags[0]!=0,"inside entry");}
    ~Inside(){flags[1]=1;if(thread){WaitForSingleObject(thread,5000);CloseHandle(thread);}}
};
struct Worker {
    std::atomic<bool> stop{false}; std::atomic<unsigned> ticks{0}; HANDLE thread=nullptr;
    static DWORD WINAPI body(void* p){auto& w=*static_cast<Worker*>(p);while(!w.stop.load()){++w.ticks;Sleep(0);}return 0;}
    void start(){thread=CreateThread(nullptr,0,body,this,0,nullptr);need(thread!=nullptr,"outside worker");}
    ~Worker(){stop=true;if(thread){WaitForSingleObject(thread,5000);CloseHandle(thread);}}
};
enum class Fault {none,write,verify,flush,protection,restore};
// Fault injection exists only here. Both actual Site suspension and actual copy2
// Memory are used; the proxy never authorizes itself without Site::freeze().
struct FaultOps final:Operations,PatchMemory {
    Fixture& f;cap4::Site site;bool authorized=false;cap4::Memory memory;
    Fault fault;bool injected=false,wrote=false;unsigned writes=0;bool invalid_order=false;
    FaultOps(Fixture& fixture,Fault fail):f(fixture),site(f.target(),f.range(),0x0b73,Strategy::quiesced_copy2),
        memory(f.target(),0x0b73,Strategy::quiesced_copy2,&authorized),fault(fail){}
    ~FaultOps(){thaw();}
    bool prepare()noexcept override{return site.prepare();}
    bool pin_module()noexcept override{return site.pin_module();}
    void publish()noexcept override{}
    void recording(bool)noexcept override{}
    bool freeze()noexcept override{authorized=false;authorized=site.freeze();return authorized;}
    void thaw()noexcept override{authorized=false;site.thaw();}
    bool no_active_calls()noexcept override{return true;}
    PatchResult patch(bool install)noexcept override{return transact(*this,install);}
    PatchResult repair_original_auxiliaries()noexcept override{return lifecycle::repair_original_auxiliaries(*this);}
    void clear_continuation()noexcept override{}
    bool has_unwind()const noexcept override{return false;}
    bool has_allocation()const noexcept override{return false;}
    bool unregister_unwind()noexcept override{return true;}
    bool free_allocation()noexcept override{return true;}
    Bytes classify()noexcept override{if(fault==Fault::verify&&wrote&&!injected){injected=true;return Bytes::unknown;}return memory.classify();}
    bool writable()noexcept override{if(!authorized||!site.quiesced())invalid_order=true;return memory.writable();}
    bool write(Bytes b)noexcept override{
        ++writes;if(!authorized||!site.quiesced())invalid_order=true;
        if(!injected&&(fault==Fault::write||(fault==Fault::restore&&b==Bytes::original))){injected=true;return false;}
        wrote=true;return memory.write(b);
    }
    bool flush()noexcept override{if(fault==Fault::flush&&!injected){injected=true;return false;}return memory.flush();}
    bool restore_protection()noexcept override{if(fault==Fault::protection&&!injected){injected=true;return false;}return memory.restore_protection();}
};
int main(){std::setvbuf(stdout,nullptr,_IONBF,0);try{
    Fixture f;
    check((reinterpret_cast<std::uintptr_t>(f.target())&1)!=0,"actual_odd_target");
    check(f.original()&&f.run(1)==4&&f.run(9)==10,"authored_original_semantics");
    cap4::Site legacy(f.target(),f.range(),0x0b73);
    check(!legacy.prepare(),"atomic16_odd_refused");
    cap4::Memory atomic(f.target(),0x0b73);
    check(!atomic.write(Bytes::detour)&&f.original(),"atomic16_odd_write_refused");
    bool authorized=false;
    cap4::Memory memory(f.target(),0x0b73,Strategy::quiesced_copy2,&authorized);
    const auto protection=f.protection();
    check(!memory.writable()&&f.protection()==protection,"unquiesced_writable_no_protection_change");
    check(!memory.write(Bytes::detour)&&f.original(),"unquiesced_write_zero_bytes");
    auto denied=transact(memory,true);
    check(!denied.committed&&!denied.write_attempted&&f.original()&&f.protection()==protection,"unquiesced_full_transaction_refused");
    cap4::Memory missing(f.target(),0x0b73,Strategy::quiesced_copy2);
    check(!missing.writable()&&!missing.write(Bytes::detour),"missing_authorization_refused");
    cap4::Site site(f.target(),f.range(),0x0b73,Strategy::quiesced_copy2);
    check(site.prepare(),"copy2_prepare");
    need(site.freeze(),"explicit native freeze");
    check(site.quiesced(),"native_freeze_authorization");
    auto installed=site.patch(true);
    check(installed.committed&&installed.after==Bytes::detour&&installed.cache_synchronized&&installed.protection_restored&&f.patched(),"copy2_exact_install");
    site.thaw();
    check(!site.quiesced()&&f.protection()==protection&&f.run(9)==4,"thaw_revoked_and_original_continuation");
    auto unquiesced_restore=site.patch(false);
    check(!unquiesced_restore.committed&&!unquiesced_restore.write_attempted&&f.patched(),"unquiesced_restore_refused");
    need(site.freeze(),"fresh restore freeze");auto restored=site.patch(false);site.thaw();
    check(restored.committed&&restored.after==Bytes::original&&f.original()&&!site.quiesced()&&f.run(9)==10,"fresh_frozen_restore");
    {Inside worker(f);cap4::Site blocked(f.target(),f.range(),0x0b73,Strategy::quiesced_copy2);Controller c;
        check(!c.install(true,blocked)&&!blocked.quiesced()&&!c.may_be_live()&&!c.last_patch().write_attempted&&f.original()&&f.protection()==protection,"real_worker_inside_range_refused");
        check(!blocked.patch(true).write_attempted&&f.original(),"failed_freeze_does_not_authorize");
        worker.flags[1]=1;check(WaitForSingleObject(worker.thread,5000)==WAIT_OBJECT_0,"inside_worker_joined");}
    {Worker w[3];for(auto& x:w)x.start();Sleep(10);
        cap4::Site safe(f.target(),f.range(),0x0b73,Strategy::quiesced_copy2);
        need(safe.freeze(),"safe worker freeze");check(safe.quiesced(),"multiworker_authorized");
        need(safe.patch(true).committed&&safe.patch(false).committed,"multiworker transaction");
        unsigned before[3];for(unsigned i=0;i<3;++i)before[i]=w[i].ticks.load();safe.thaw();
        const auto end=GetTickCount64()+5000;bool progressed=false;
        do{progressed=true;for(unsigned i=0;i<3;++i)progressed&=w[i].ticks.load()>before[i];if(!progressed)Sleep(1);}while(!progressed&&GetTickCount64()<end);
        check(progressed&&!safe.quiesced()&&f.original(),"multiworker_thaw_progress");
        for(auto& x:w)x.stop=true;
        bool joined=true;for(auto& x:w)joined&=WaitForSingleObject(x.thread,5000)==WAIT_OBJECT_0;
        check(joined,"outside_workers_joined");}
    for(auto fault:{Fault::write,Fault::verify,Fault::flush,Fault::protection}){
        FaultOps ops(f,fault);Controller c;need(!c.install(true,ops),"injected install accepted");
        if(fault==Fault::verify){need(c.phase()==Phase::containment&&!c.release_allowed()&&f.patched(),"verification containment");need(c.stop(ops),"verified frozen retry");}
        check(f.original()&&!ops.authorized&&!ops.invalid_order&&f.protection()==protection,
            fault==Fault::write?"write_failure":fault==Fault::verify?"verification_containment":fault==Fault::flush?"cache_failure_rollback":"protection_failure_rollback");
        if(fault==Fault::flush||fault==Fault::protection)need(c.last_patch().rollback_attempted&&ops.writes==2,"rollback missing");
    }
    {FaultOps ops(f,Fault::restore);Controller c;need(c.install(true,ops),"restore setup");
        check(!c.stop(ops)&&c.phase()==Phase::containment&&!c.release_allowed()&&f.patched()&&!ops.authorized,"restore_failure_containment");
        check(c.stop(ops)&&f.original()&&!ops.invalid_order,"containment_fresh_freeze_retry");}
    {DWORD old=0;need(VirtualProtect(f.base+4096,4096,PAGE_NOACCESS,&old)!=0,"boundary setup");
        cap4::Site boundary(f.base+4095,{reinterpret_cast<std::uintptr_t>(f.base),8192},0x0b73,Strategy::quiesced_copy2);
        check(!boundary.prepare(),"unreadable_second_byte_refused");
        need(VirtualProtect(f.base+4096,4096,old,&old)!=0,"boundary restore");}
    cap4::Site outside(f.target(),{reinterpret_cast<std::uintptr_t>(f.target()),1},0x0b73,Strategy::quiesced_copy2);
    check(!outside.prepare(),"second_byte_outside_forbidden_range_refused");
    cap4::Site wrong(f.target(),f.range(),0x0a73,Strategy::quiesced_copy2);
    check(!wrong.prepare(),"wrong_original_refused");
    cap4::Site invalid(f.target(),f.range(),0x0b73,static_cast<Strategy>(99));
    check(!invalid.prepare(),"unknown_strategy_refused");
    for(const auto* d:{&fix_builds::ets157,&fix_builds::ets158,&fix_builds::ets159})check(d->cap4_write_strategy==Strategy::aligned_atomic16,"supported_descriptor_remains_atomic16");
    check(fix_builds::supported.size()==4&&fix_builds::ets160.cap4_write_strategy==Strategy::quiesced_copy2,"four_exact_builds_160_copy2");
    DWORD handles_before=0,handles_after=0;need(GetProcessHandleCount(GetCurrentProcess(),&handles_before)!=0,"handles before");
    for(unsigned i=0;i<1000;++i){cap4::Site s(f.target(),f.range(),0x0b73,Strategy::quiesced_copy2);Controller c;
        need(f.original()&&!s.quiesced(),"cycle start");
        need(c.install(true,s)&&f.patched()&&!s.quiesced(),"cycle install");
        need(c.stop(s)&&f.original()&&!s.quiesced()&&c.release_allowed()&&f.protection()==protection,"cycle restore");
        if((i+1)%100==0)std::printf("STRESS_PROGRESS %u/1000\n",i+1);}
    check(f.original(),"1000_complete_native_cycles");
    need(GetProcessHandleCount(GetCurrentProcess(),&handles_after)!=0,"handles after");
    check(handles_before==handles_after&&f.protection()==protection,"no_handle_or_protection_leak");
    check(!site.quiesced()&&f.neighbors(),"final_authorization_and_neighbors_clean");
    std::printf("COPY2_TOTAL %u\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL COPY2 %s\n",e.what());return 1;}}
