#pragma once
#include <cstdint>
#include <string_view>

namespace cmf::fix_builds {
// Exact supported executable descriptors shared by both behavior patches.
struct Descriptor {
    const char* version;
    const char* sha256;
    std::uint32_t cap4, generator_begin, generator_end;
    std::uint32_t spread, sweep_begin, sweep_end, sweep_call_displacement;
    std::uint32_t bulk_call, premutation;
};
inline constexpr Descriptor ets157{
    "1.57.2.7", "06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9",
    0x006CE618,0x006CE3F0,0x006CEE5B,
    0x003EC647,0x003EC330,0x003EC677,0x002E0075,0x003ED746,0x006CC708};
inline constexpr Descriptor ets158{
    "1.58.1.4", "25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644",
    0x006F4CE8,0x006F4AC0,0x006F563C,
    0x00407627,0x00407310,0x00407657,0x002EB765,0x004086BB,0x006F2DD8};
inline const Descriptor* identify(std::string_view hash) noexcept {
    if(hash==ets157.sha256)return &ets157;
    if(hash==ets158.sha256)return &ets158;
    return nullptr;
}
inline bool known(const Descriptor* d) noexcept {return d==&ets157||d==&ets158;}
}
