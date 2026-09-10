// Test-only deterministic Windows operation failures around real Site suspension,
// relay allocation and exact eight-byte WindowsPatchMemory transactions.
enum class Inject {prepare,writable,write,flush,protection,restore,none};
struct SpreadFault final:cmf::lifecycle::Operations,cmf::lifecycle::PatchMemory {
    cmf::spread::Site site;std::array<std::uint8_t,8> detour{};DWORD protection=0;bool pending=false;
    cmf::WindowsPatchMemory memory;Inject inject;bool once=false,frozen=false;unsigned writes=0;bool bad_order=false;
    SpreadFault(unsigned char* p,cmf::MemoryRange r,Inject i):site(p,r,cmf::spread::v160_layout),
        memory(p,cmf::spread::v160_layout.original.data(),detour.data(),8,protection,pending),inject(i){}
    bool prepare()noexcept override{
        if(inject==Inject::prepare)return false;
        if(!site.prepare())return false;
        detour={0xe8,0,0,0,0,0xe3,0x15,0x90};
        auto delta=static_cast<std::int32_t>(site.relay_range().begin-reinterpret_cast<std::uintptr_t>(&s160_site+5));
        std::memcpy(detour.data()+1,&delta,4);return true;
    }
    bool pin_module()noexcept override{return site.pin_module();}
    void publish()noexcept override{}
    void recording(bool)noexcept override{}
    bool freeze()noexcept override{frozen=site.freeze();return frozen;}
    void thaw()noexcept override{frozen=false;site.thaw();}
    bool no_active_calls()noexcept override{return site.no_active_calls();}
    cmf::lifecycle::PatchResult patch(bool b)noexcept override{return cmf::lifecycle::transact(*this,b);}
    cmf::lifecycle::PatchResult repair_original_auxiliaries()noexcept override{return cmf::lifecycle::repair_original_auxiliaries(*this);}
    void clear_continuation()noexcept override{}
    bool has_unwind()const noexcept override{return false;}
    bool has_allocation()const noexcept override{return site.has_allocation();}
    bool unregister_unwind()noexcept override{return true;}
    bool free_allocation()noexcept override{return site.free_allocation();}
    cmf::lifecycle::Bytes classify()noexcept override{return memory.classify();}
    bool writable()noexcept override{bad_order|=!frozen;if(inject==Inject::writable&&!once){once=true;return false;}return memory.writable();}
    bool write(cmf::lifecycle::Bytes b)noexcept override{
        ++writes;bad_order|=!frozen;
        if(!once&&(inject==Inject::write||(inject==Inject::restore&&b==cmf::lifecycle::Bytes::original))){once=true;return false;}return memory.write(b);
    }
    bool flush()noexcept override{if(inject==Inject::flush&&!once){once=true;return false;}return memory.flush();}
    bool restore_protection()noexcept override{if(inject==Inject::protection&&!once){once=true;return false;}return memory.restore_protection();}
};
