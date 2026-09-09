#include "cap4_patch.h"
#include "logger.h"
#include "build_info.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
using U=unsigned long long;
extern "C" {
U cap_fixture(U,void(*)(U)=nullptr,void* =nullptr);
extern unsigned char cap_site,cap_begin,cap_continue,cap_fault,cap_end;
U cap_registers[16]{},cap_rsp_before{},cap_rsp_after{};
alignas(16) unsigned char cap_fx[512]{},cap_fx_before[512]{};
}
static unsigned count=0;
static void need(bool b,const char* name){if(!b)throw std::runtime_error(name);}
static void check(bool b,const char* name){need(b,name);++count;std::printf("PASS %s\n",name);}
static cmf::MemoryRange range(){return {reinterpret_cast<std::uintptr_t>(&cap_fixture),
    reinterpret_cast<std::uintptr_t>(&cap_end)-reinterpret_cast<std::uintptr_t>(&cap_fixture)};}
static DWORD WINAPI idle(void*){return 0;}
static bool thread_case(void* ip,bool rejected){
    HANDLE t=CreateThread(nullptr,0,idle,nullptr,CREATE_SUSPENDED,nullptr);need(t!=nullptr,"thread");
    CONTEXT original{};original.ContextFlags=CONTEXT_CONTROL;need(GetThreadContext(t,&original)!=0,"thread context");
    auto altered=original;altered.Rip=reinterpret_cast<DWORD64>(ip);need(SetThreadContext(t,&altered)!=0,"authored IP");
    cmf::cap4::Site s(&cap_site,range());cmf::lifecycle::Controller c;
    const bool installed=c.install(true,s);if(installed)need(c.stop(s),"thread test restore");
    need(SetThreadContext(t,&original)!=0,"restore real IP");need(ResumeThread(t)!=DWORD(-1),"resume");
    need(WaitForSingleObject(t,10000)==WAIT_OBJECT_0,"join");CloseHandle(t);
    return installed!=rejected&&cap_site==0x73;
}
static unsigned cleanup=0,searches=0,resumes=0;static void* fault_pc=nullptr;
static LONG CALLBACK search(EXCEPTION_POINTERS* p){
    if(p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION)++searches;
    if(p->ExceptionRecord->ExceptionCode==0xe0410404){++resumes;return EXCEPTION_CONTINUE_EXECUTION;}
    return EXCEPTION_CONTINUE_SEARCH;
}
static int filter(EXCEPTION_POINTERS* p){fault_pc=p->ExceptionRecord->ExceptionAddress;
    return p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}
static void fault(void* bad){__try{__try{cap_fixture(10,nullptr,bad);}__finally{++cleanup;}}
    __except(filter(GetExceptionInformation())){}}
static void thrower(U){throw 404;}
static void continuer(U){RaiseException(0xe0410404,0,0,nullptr);}
struct Cleaner {~Cleaner(){++cleanup;}};
static void cpp(){Cleaner c;cap_fixture(10,thrower,nullptr);}
struct RestoreFault final:cmf::lifecycle::PatchMemory {
    cmf::cap4::Memory memory;bool once=true;
    explicit RestoreFault(std::uint8_t* p):memory(p){}
    cmf::lifecycle::Bytes classify() noexcept override{return memory.classify();}
    bool writable() noexcept override{return memory.writable();}
    bool write(cmf::lifecycle::Bytes b) noexcept override{return memory.write(b);}
    bool flush() noexcept override{return memory.flush();}
    bool restore_protection() noexcept override{if(once){once=false;return false;}return memory.restore_protection();}
};
int main(int argc,char** argv){try{
    need(argc==4,"disabled INI, release INI and read-only executable required");
    auto defaults=cmf::load_config(argv[1]),enabled=cmf::load_config(argv[2]);
    check(!defaults.cap4_install&&!defaults.cap4_enabled,"CP01_default_disabled");
    check(enabled.cap4_install&&enabled.cap4_enabled&&!enabled.cap4_refusal(),"CP02_enabled_example_parsed");
    auto c=enabled;c.cap4_install=false;check(c.cap4_refusal()!=nullptr,"CP03_install_false");
    c=enabled;c.cap4_enabled=false;check(c.cap4_refusal()!=nullptr,"CP04_enabled_false");
    c=enabled;c.cap4_config_valid=false;check(c.cap4_refusal()!=nullptr,"CP05_malformed_refused");
    c=enabled;c.hooking_enabled=true;check(c.cap4_refusal()!=nullptr,"CP06_observation_mode_conflict");
    c=enabled;c.install_cmf_internal_compatibility=true;check(c.cap4_refusal()!=nullptr,"CP07_compatibility_conflict");
    c=enabled;c.membership_shadow_install=true;check(c.cap4_refusal()!=nullptr,"CP08_shadow_conflict");
    c=enabled;c.allow_unknown_build=true;check(c.cap4_refusal()!=nullptr,"CP09_unknown_build_config");
    c=enabled;c.fail_closed=false;check(c.cap4_refusal()!=nullptr,"CP10_fail_closed_required");
    cmf::Logger log;cmf::Cap4Patch owner;
    check(owner.start(defaults,false,log)&&!owner.potentially_live()&&!cmf::hook_module_was_pinned(),"CP11_default_no_scan_or_PIN");
    check(!owner.start(enabled,false,log)&&!owner.potentially_live()&&!cmf::hook_module_was_pinned(),"CP12_wrong_hash_no_scan_or_PIN");
    check(owner.stop(log)&&owner.stop(log),"CP13_empty_repeated_stop");
    constexpr std::size_t size=0x700000;
    auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));need(image,"test image");
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+0x100);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt->FileHeader.NumberOfSections=1;
    nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=size;
    auto* section=IMAGE_FIRST_SECTION(nt);std::memcpy(section->Name,".text",5);
    section->VirtualAddress=0x6CE000;section->Misc.VirtualSize=0x2000;section->Characteristics=IMAGE_SCN_MEM_EXECUTE;
    std::memcpy(image+cmf::cap4::target_rva-19,cmf::cap4::signature.data(),67);
    check(cmf::cap4::validate_image(image,size),"CP14_exact_loaded_image_identity");
    image[cmf::cap4::target_rva]=0x72;check(!cmf::cap4::validate_image(image,size),"CP15_opcode_mismatch");image[cmf::cap4::target_rva]=0x73;
    std::memcpy(image+0x6CE700,cmf::cap4::signature.data(),67);check(!cmf::cap4::validate_image(image,size),"CP16_duplicate_signature");std::memset(image+0x6CE700,0,67);
    section->Misc.VirtualSize=size;check(!cmf::cap4::validate_image(image,size),"CP17_PE_bounds_refusal");section->Misc.VirtualSize=0x2000;
    VirtualFree(image,0,MEM_RELEASE);
    HANDLE f=CreateFileW(std::filesystem::path(argv[3]).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    need(f!=INVALID_HANDLE_VALUE,"readonly game file");
    HANDLE m=CreateFileMappingW(f,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);need(m!=nullptr,"nonexecuting image mapping");
    auto* mapped=static_cast<const std::uint8_t*>(MapViewOfFile(m,FILE_MAP_READ,0,0,0));need(mapped,"readonly view");
    const auto* md=reinterpret_cast<const IMAGE_DOS_HEADER*>(mapped);const auto* mn=reinterpret_cast<const IMAGE_NT_HEADERS64*>(mapped+md->e_lfanew);
    const auto identity=cmf::inspect_executable(argv[3]);
    const auto* build=cmf::fix_builds::identify(identity.sha256);
    check(identity.sha256_available&&build&&cmf::cap4::validate_image(mapped,mn->OptionalHeader.SizeOfImage,build),"CP18_real_image_readonly_NO_EXECUTE_identity");
    UnmapViewOfFile(mapped);CloseHandle(m);CloseHandle(f);
    cmf::cap4::Site s(&cap_site,range());cmf::lifecycle::Controller controller;
    check(cap_fixture(10)==10,"CP19_original_semantics");
    check(controller.install(true,s)&&controller.module_pinned(),"CP20_production_site_native_install_PIN");
    check(cap_site==0x90&&*(&cap_site+1)==0x90,"CP21_exact_NOPS");
    bool fixed=true;for(U q=0;q<=11;++q)fixed&=cap_fixture(q)==4;
    check(fixed,"CP22_fixed_four_all_candidate_counts");
    DWORD64 base=0;auto* rf=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(&cap_site),&base,nullptr);need(rf,"unwind");auto original_rf=*rf;
    auto* bad=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS);need(bad,"fault page");auto veh=AddVectoredExceptionHandler(1,search);need(veh,"VEH");
    fault(bad);check(cleanup==1&&searches==1&&fault_pc==&cap_fault,"CP23_original_SEH_fault_and_cleanup");
    try{cpp();need(false,"missing C++ throw");}catch(int v){need(v==404,"exception value");}
    check(cleanup==2,"CP24_CPP_cleanup_once");
    check(cap_fixture(10,continuer)==4&&resumes==1,"CP25_continue_execution");
    check(controller.stop(s)&&controller.original_verified()&&cap_fixture(10)==10,"CP26_exact_restore_original_execution");
    check(!std::memcmp(rf,&original_rf,sizeof(original_rf)),"CP27_unwind_metadata_untouched");
    check(controller.stop(s),"CP28_repeated_stop");
    RemoveVectoredExceptionHandler(veh);VirtualFree(bad,0,MEM_RELEASE);
    MEMORY_BASIC_INFORMATION before{},after{};VirtualQuery(&cap_site,&before,sizeof(before));
    RestoreFault faulty(&cap_site);auto outcome=cmf::lifecycle::transact(faulty,true);VirtualQuery(&cap_site,&after,sizeof(after));
    check(!outcome.committed&&outcome.rollback_attempted&&outcome.rollback_original_verified&&outcome.protection_restored&&outcome.cache_synchronized&&before.Protect==after.Protect,"CP29_production_backend_protection_rollback");
    auto* cell=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));need(cell,"test cell");cell[0]=0x73;cell[1]=0x0a;
    cmf::cap4::Site sim(cell,{reinterpret_cast<std::uintptr_t>(cell),2});cmf::lifecycle::Controller sc;
    need(sc.install(true,sim),"cell install");cell[0]=0xcc;
    check(!sc.stop(sim)&&sc.phase()==cmf::lifecycle::Phase::containment&&!sc.release_allowed()&&cell[0]==0xcc,"CP30_unknown_owner_containment_no_overwrite");
    check(!sc.install(true,sim),"CP31_containment_reinstall_refused");VirtualFree(cell,0,MEM_RELEASE);
    check(thread_case(&cap_begin,true),"CP32_thread_before_refusal");
    check(thread_case(&cap_site,true),"CP33_thread_at_refusal");
    check(thread_case(&cap_continue,true),"CP34_thread_after_refusal");
    check(thread_case(&cap_fault,true),"CP35_containing_region_refusal");
    check(thread_case(reinterpret_cast<void*>(&idle),false),"CP36_unrelated_thread_allowed");
    check(cap_fixture(10)==10,"CP37_final_original_clean");
    std::printf("CAP4_PRODUCTION_TOTAL %u\n",count);return 0;
}catch(const std::exception& e){std::printf("FAIL CAP4_production %s\n",e.what());return 1;}}
