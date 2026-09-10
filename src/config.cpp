#include "config.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cwctype>
#include <string>

namespace cmf {
namespace {
std::wstring read(const std::filesystem::path& path, const wchar_t* section,
                  const wchar_t* key, const wchar_t* fallback = L"") {
    std::array<wchar_t, 256> buffer{};
    const auto n = GetPrivateProfileStringW(section, key, fallback, buffer.data(),
                                           static_cast<DWORD>(buffer.size()), path.c_str());
    if (n >= buffer.size() - 1) return L"__invalid__";
    std::wstring value(buffer.data());
    const auto space = [](wchar_t c) { return std::iswspace(c) != 0; };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), space));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), space).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}
bool boolean(const std::filesystem::path& path, const wchar_t* section,
             const wchar_t* key, bool& valid, bool optional = false) {
    const auto text = read(path, section, key, optional ? L"false" : L"");
    if (text == L"true" || text == L"yes" || text == L"on" || text == L"1") return true;
    if (text == L"false" || text == L"no" || text == L"off" || text == L"0") return false;
    valid = false;
    return false;
}
}
PluginConfig load_config(const std::filesystem::path& path) noexcept {
    PluginConfig c;
    try {
        if (path.empty() || !std::filesystem::is_regular_file(path)) return c;
        c.ini_found = true;
        bool valid = true;
        c.enabled = boolean(path, L"General", L"enabled", valid);
        c.cap4_install = boolean(path, L"CMF_CAP4", L"cap4_install", valid);
        c.cap4_enabled = boolean(path, L"CMF_CAP4", L"cap4_enabled", valid);
        c.refresh_spread_install = boolean(path, L"CMF_REFRESH_SPREAD", L"refresh_spread_install", valid);
        c.refresh_spread_enabled = boolean(path, L"CMF_REFRESH_SPREAD", L"refresh_spread_enabled", valid);
        const auto period = read(path, L"CMF_REFRESH_SPREAD", L"refresh_spread_minutes");
        if (period == L"60") c.refresh_spread_minutes = 60;
        else if (period == L"120") c.refresh_spread_minutes = 120;
        else if (period == L"180") c.refresh_spread_minutes = 180;
        else valid = false;
        // These safety gates are fixed in this release; old overrides fail closed.
        bool safety_valid = true;
        c.target_this_build_only = boolean(path, L"Build", L"target_this_build_only", safety_valid, false);
        c.allow_unknown_build = boolean(path, L"Build", L"allow_unknown_build", safety_valid, false);
        c.fail_closed = boolean(path, L"Hooking", L"fail_closed", safety_valid, false);
        struct Legacy { const wchar_t* section; const wchar_t* key; bool* value; };
        const Legacy legacy[] = {
            {L"Hooking", L"enabled", &c.hooking_enabled},
            {L"Hooking", L"install_cmf_re_001", &c.install_cmf_re_001},
            {L"Hooking", L"install_cmf_re_003", &c.install_cmf_re_003},
            {L"Hooking", L"install_cmf_internal_candidate", &c.install_cmf_internal_candidate},
            {L"Hooking", L"install_cmf_internal_destination", &c.install_cmf_internal_destination},
            {L"Hooking", L"install_cmf_internal_compatibility", &c.install_cmf_internal_compatibility},
            {L"CMF_RE_001", L"enabled", &c.cmf_re_001_enabled},
            {L"CMF_RE_003", L"enabled", &c.cmf_re_003_enabled},
            {L"CMF_INTERNAL_CANDIDATE", L"enabled", &c.cmf_internal_candidate_enabled},
            {L"CMF_INTERNAL_DESTINATION", L"enabled", &c.cmf_internal_destination_enabled},
            {L"CMF_INTERNAL_COMPATIBILITY", L"enabled", &c.cmf_internal_compatibility_enabled},
            {L"CMF_MEMBERSHIP_SHADOW", L"membership_shadow_install", &c.membership_shadow_install},
            {L"CMF_MEMBERSHIP_SHADOW", L"membership_shadow_enabled", &c.membership_shadow_enabled}
        };
        for (const auto& item : legacy) *item.value = boolean(path, item.section, item.key, valid, true);
        c.cap4_config_valid = valid;
        c.refresh_spread_config_valid = valid;
        c.hook_config_valid = valid && safety_valid;
    } catch (...) {
        c.cap4_config_valid = c.refresh_spread_config_valid = c.hook_config_valid = false;
    }
    return c;
}
}
