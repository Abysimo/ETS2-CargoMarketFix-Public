#include "refresh_spread.h"
#include "cap4_patch.h"
#include "logger.h"
#include "build_info.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
struct Economy {unsigned char pad[0x15c]{};std::uint32_t minute=0;};
struct Company {unsigned visited=0;bool expired=true;};
extern "C" {
void spread_fixture(Company**,std::uint64_t,Economy*);
void spread_probe(Economy*);
extern unsigned char spread_site,spread_loop,spread_finish,spread_fixture_end;
std::uint64_t spread_registers[10]{},spread_rsp_before{},spread_rsp_after{};
alignas(16) unsigned char spread_fx_before[512]{},spread_fx_after[512]{};
}
static unsigned cases=0,mode=0,cleanup=0,searches=0;
static void need(bool v,const char* s){if(!v)throw std::runtime_error(s);}
static void check(bool v,const char* s){need(v,s);++cases;std::printf("PASS %s\n",s);}
extern "C" void spread_test_visit(Company* c,Economy*) {
    if(mode==1)throw 6060;
    if(mode==2)RaiseException(0xe0600060,0,0,nullptr);
    ++c->visited;c->expired=false;
}
static LONG CALLBACK search(EXCEPTION_POINTERS* p){
    if(p->ExceptionRecord->ExceptionCode==0xe0600060){++searches;return EXCEPTION_CONTINUE_EXECUTION;}
    return EXCEPTION_CONTINUE_SEARCH;
}
static void fault_call(Company** c,Economy* e){
    __try {__try {spread_fixture(c,60,e);}__finally {++cleanup;}}
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}
}
static cmf::MemoryRange range(){auto b=reinterpret_cast<std::uintptr_t>(&spread_fixture);
    return {b,reinterpret_cast<std::uintptr_t>(&spread_fixture_end)-b};}
static DWORD WINAPI idle(void*){return 0;}
static bool blocks(cmf::spread::Site& site,void* ip){
    HANDLE t=CreateThread(nullptr,0,idle,nullptr,CREATE_SUSPENDED,nullptr);need(t,"synthetic thread");
    CONTEXT saved{};saved.ContextFlags=CONTEXT_CONTROL;need(GetThreadContext(t,&saved)!=0,"context");
    auto changed=saved;changed.Rip=reinterpret_cast<DWORD64>(ip);need(SetThreadContext(t,&changed)!=0,"authored IP");
    const bool acquired=site.freeze();if(acquired)site.thaw();
    need(SetThreadContext(t,&saved)!=0&&ResumeThread(t)!=DWORD(-1),"resume original thread");
    need(WaitForSingleObject(t,10000)==WAIT_OBJECT_0,"join");CloseHandle(t);return !acquired;
}
static void cycle(std::size_t n,unsigned period=60){
    std::vector<Company> companies(n);std::vector<Company*> pointers;for(auto& c:companies)pointers.push_back(&c);
    std::size_t expected_begin=0,small=n,big=0;
    for(unsigned b=0;b<period;++b){Economy e;e.minute=b;
        spread_fixture(pointers.data(),n,&e);
        const auto size=n/period+(b<n%period),end=expected_begin+size;
        for(std::size_t i=0;i<n;++i)need(companies[i].visited==(i<end?1u:0u),"ordered unique contiguous coverage");
        expected_begin=end;small=std::min(small,size);big=std::max(big,size);
    }
    need(expected_begin==n&&big-small<=1,"complete balanced partition");
}
int main(int argc,char** argv){try{
    need(argc==4,"disabled INI, active INI and read-only executable required");
    const auto defaults=cmf::load_config(argv[1]),active=cmf::load_config(argv[2]);
    check(!defaults.refresh_spread_install&&!defaults.refresh_spread_enabled,"RS01_defaults_disabled");
    check(!active.refresh_spread_refusal()&&!active.cap4_refusal(),"RS02_active_CAP4_and_spread");
    auto c=active;c.refresh_spread_minutes=59;check(c.refresh_spread_refusal()!=nullptr,"RS03_invalid_59_refused");
    check(active.refresh_spread_minutes==180&&!active.refresh_spread_refusal()&&!active.cap4_refusal(),"RS44_active_180_example");
    {wchar_t dir[MAX_PATH]{},name[MAX_PATH]{};
        need(GetTempPathW(MAX_PATH,dir)&&GetTempFileNameW(dir,L"cmf",0,name),"config fixture path");
        struct Temp {const wchar_t* path;~Temp(){DeleteFileW(path);}} temp{name};
        need(CopyFileW(std::filesystem::path(argv[2]).c_str(),name,FALSE)!=0,"config fixture copy");
        for(const auto* token:{L"60",L"120",L"180"}){
            need(WritePrivateProfileStringW(L"CMF_REFRESH_SPREAD",L"refresh_spread_minutes",token,name)!=0,"author valid token");
            const auto parsed=cmf::load_config(name);
            need(!parsed.refresh_spread_refusal()&&parsed.refresh_spread_minutes==std::stoul(token),"valid period exact");
        }
        check(true,"RS38_exact_60_120_180_config");
        for(const auto* token:{L"0",L"59",L"61",L"119",L"121",L"179",L"181",L"240",L"-120",L"120x",L"180x",L"0180",L"060",L""}){
            need(WritePrivateProfileStringW(L"CMF_REFRESH_SPREAD",L"refresh_spread_minutes",token,name)!=0,"author invalid token");
            const auto bad=cmf::load_config(name);need(!bad.refresh_spread_config_valid&&bad.refresh_spread_refusal(),"invalid period token fail closed");}
        need(WritePrivateProfileStringW(L"CMF_REFRESH_SPREAD",L"refresh_spread_minutes",nullptr,name)!=0,"missing period");
        need(cmf::load_config(name).refresh_spread_refusal()!=nullptr,"missing period fail closed");
        check(true,"RS39_invalid_and_missing_period_tokens_fail_closed");}
    c=active;c.refresh_spread_install=false;check(c.refresh_spread_refusal()!=nullptr,"RS04_both_gates");
    c=active;c.refresh_spread_config_valid=false;check(c.refresh_spread_refusal()!=nullptr,"RS05_invalid_config");
    c=active;c.cap4_enabled=false;check(c.refresh_spread_refusal()!=nullptr,"RS06_CAP4_required");
    c=active;c.membership_shadow_install=true;check(c.refresh_spread_refusal()!=nullptr,"RS07_observers_rejected");
    cmf::Logger log;cmf::RefreshSpread owner;
    check(owner.start(defaults,false,log)&&owner.stop(log),"RS08_default_no_install");
    check(!owner.start(active,false,log),"RS09_exact_build_required");
    HANDLE f=CreateFileW(std::filesystem::path(argv[3]).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    need(f!=INVALID_HANDLE_VALUE,"read-only target");
    HANDLE m=CreateFileMappingW(f,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);need(m,"nonexecuting map");
    auto* image=static_cast<const std::uint8_t*>(MapViewOfFile(m,FILE_MAP_READ,0,0,0));need(image,"view");
    auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
    const auto identity=cmf::inspect_executable(std::filesystem::path(argv[3]).c_str());
    const auto* build=cmf::fix_builds::identify(identity.sha256);need(identity.sha256_available&&build,"exact build descriptor");
    check(cmf::spread::validate_image(image,nt->OptionalHeader.SizeOfImage,build),"RS10_real_exact_sweep_signature");
    check(cmf::cap4::validate_image(image,nt->OptionalHeader.SizeOfImage,build),"RS11_CAP4_target_unchanged");
    check(image[build->bulk_call]==0xe8&&image[build->premutation]==0x48,"RS12_bulk_and_premutation_sites_unmodified");
    UnmapViewOfFile(image);CloseHandle(m);CloseHandle(f);
    check(&spread_loop-&spread_site==9&&&spread_finish-&spread_loop==0x14,"RS13_native_layout_original_continuations");
    std::vector<Company> companies(60);std::vector<Company*> ptr;for(auto& x:companies)ptr.push_back(&x);
    Economy e;
    spread_fixture(ptr.data(),60,&e);bool all=true;for(auto& x:companies)all&=x.visited==1;
    check(all,"RS14_unpatched_original_full_sweep");
    cmf::spread::Site site(&spread_site,range());cmf::lifecycle::Controller ctl;
    check(ctl.install(true,site)&&ctl.module_pinned(),"RS15_native_install_PIN");
    check(spread_site==0xe8&&*(&spread_site+5)==0xe3&&*(&spread_site+6)==0x16,"RS16_CALL_RET_JRCXZ_no_return_rewriting");
    check(blocks(site,&spread_loop),"RS35_quiescence_whole_normal_caller");
    check(blocks(site,reinterpret_cast<void*>(site.relay_range().begin)),"RS36_quiescence_relay");
    check(blocks(site,reinterpret_cast<void*>(&cmf_refresh_spread_bridge)),"RS37_quiescence_bridge");
    cycle(0);check(true,"RS17_zero_companies");
    cycle(1);cycle(17);cycle(59);check(true,"RS18_N_less_than_60_empty_buckets");
    cycle(60);check(true,"RS19_N_equals_60");
    cycle(61);cycle(12921);check(true,"RS20_nondivisible_N_exact_once_balance");
    cycle(400);cycle(7);cycle(600);check(true,"RS21_changing_N_no_retained_state");
    for(auto& x:companies)x=Company{};e.minute=0;spread_fixture(ptr.data(),60,&e);
    check(!companies[0].expired&&companies[59].expired&&companies[59].visited==0,"RS22_skipped_expired_state_retained");
    e.minute=59;spread_fixture(ptr.data(),60,&e);
    check(!companies[59].expired&&companies[59].visited==1,"RS23_recoverable_59_minutes_later");
    e.minute=121;spread_fixture(ptr.data(),60,&e);
    check(companies[1].visited==1,"RS24_minute_modulo_and_reset");
    e.minute=7;spread_probe(&e);
    check(spread_registers[0]==60&&spread_registers[1]==1&&
        spread_registers[2]==0x12345678&&spread_registers[3]==0x123456789&&spread_registers[4]==0x987654321&&
        spread_registers[5]==0x456789012&&spread_registers[6]==0x567890123&&
        spread_registers[7]==reinterpret_cast<std::uintptr_t>(&e)&&
        spread_registers[8]==0x10038&&spread_registers[9]==0x10040,"RS25_GPR_live_state_and_selected_bounds");
    check(spread_rsp_before==spread_rsp_after&&!std::memcmp(spread_fx_before,spread_fx_after,512),"RS26_RSP_XMM_FP_MXCSR");
    mode=1;bool caught=false;try{spread_fixture(ptr.data(),60,&e);}catch(int x){caught=x==6060;}
    check(caught&&cmf_refresh_spread_active==0,"RS27_original_CXX_exception_unwind");
    mode=2;auto handler=AddVectoredExceptionHandler(1,search);need(handler,"VEH");spread_fixture(ptr.data(),60,&e);
    RemoveVectoredExceptionHandler(handler);mode=0;
    check(searches==1&&cmf_refresh_spread_active==0,"RS28_continue_execution_original_call_once");
    fault_call(ptr.data(),&e);check(cleanup==1&&cmf_refresh_spread_active==0,"RS29_normal_finally_clean");
    check(ctl.stop(site)&&std::memcmp(&spread_site,cmf::spread::original.data(),9)==0,"RS30_exact_restore");
    for(auto& x:companies)x=Company{};spread_fixture(ptr.data(),60,&e);all=true;for(auto& x:companies)all&=x.visited==1;
    check(all,"RS31_restored_full_sweep");
    for(const auto period:{60u,120u,180u}){
        cmf::spread::Site selected(&spread_site,range(),cmf::spread::legacy_layout,period);cmf::lifecycle::Controller selected_owner;
        need(selected_owner.install(true,selected)&&cmf_refresh_spread_bucket_count==static_cast<LONG>(period),"configured period published before patch");
        for(const auto n:{0u,1u,59u,60u,61u,119u,120u,121u,179u,180u,181u,12921u}){
            cycle(n,period);check(true,("RS40_legacy_"+std::to_string(period)+"_companies_"+std::to_string(n)).c_str());}
        for(const auto minute:{0u,period-1,period,period+1,1000000u,UINT32_MAX}){
            for(auto& x:companies)x=Company{};e.minute=minute;spread_fixture(ptr.data(),60,&e);
            const auto bucket=minute%period,first=bucket*(60/period)+std::min(bucket,60%period),length=60/period+(bucket<60%period);
            for(unsigned i=0;i<60;++i)need(companies[i].visited==unsigned(i>=first&&i<first+length),"absolute minute exact partition");}
        check(true,("RS41_legacy_absolute_minutes_"+std::to_string(period)).c_str());
        need(selected_owner.stop(selected)&&cmf_refresh_spread_active==0,"period owner restore");
        for(auto& x:companies)x=Company{};spread_fixture(ptr.data(),60,&e);
        for(const auto& x:companies)need(x.visited==1,"original full traversal after configured restore");
        check(true,("RS42_period_restore_"+std::to_string(period)).c_str());
    }
    {const auto saved=cmf_refresh_spread_bucket_count;
        for(auto period:{0u,1u,59u,61u,119u,121u,179u,181u,240u,UINT32_MAX}){
            cmf::spread::Site invalid(&spread_site,range(),cmf::spread::legacy_layout,period);cmf::lifecycle::Controller invalid_owner;
            need(!invalid_owner.install(true,invalid)&&!invalid_owner.may_be_live()&&cmf_refresh_spread_bucket_count==saved,"invalid native period never published");}
        check(true,"RS43_invalid_native_period_rejected_before_publish");}
    cmf::spread::Site wrong(&spread_loop,range());cmf::lifecycle::Controller wc;
    check(!wc.install(true,wrong)&&!wc.may_be_live(),"RS32_wrong_prefix_no_patch");
    // An exception in the once/sweep bridge must unwind normally and retain the
    // lease/resources rather than declaring an unsafe clean restoration.
    cmf::spread::Site fault_site(&spread_site,range());cmf::lifecycle::Controller fc;
    need(fc.install(true,fault_site),"fault fixture install");
    fault_call(ptr.data(),nullptr);
    check(cleanup==2&&cmf_refresh_spread_active==1,"RS33_bridge_SEH_unwind_not_swallowed");
    check(!fc.stop(fault_site)&&fc.may_be_live()&&fc.phase()==cmf::lifecycle::Phase::containment,
        "RS34_fault_lease_contains_callable_resources_until_exit");
    std::printf("TOTAL %u named cases\n",cases);return 0;
}catch(const std::exception& e){std::printf("FAIL refresh_spread: %s\n",e.what());return 1;}}
