#pragma once
#include <cstdint>
#include <filesystem>

namespace cmf {
struct PluginConfig final {
    bool enabled = false;
    bool ini_found = false;
    bool cap4_install = false, cap4_enabled = false, cap4_config_valid = false;
    bool refresh_spread_install = false, refresh_spread_enabled = false;
    bool refresh_spread_config_valid = false;
    std::uint32_t refresh_spread_minutes = 60;
    bool hook_config_valid = false;
    bool target_this_build_only = true, allow_unknown_build = false, fail_closed = true;

    // Rejection-only compatibility with old INIs. No observer is compiled.
    bool hooking_enabled = false;
    bool install_cmf_re_001 = false, install_cmf_re_003 = false;
    bool install_cmf_internal_candidate = false, install_cmf_internal_destination = false;
    bool install_cmf_internal_compatibility = false;
    bool cmf_re_001_enabled = false, cmf_re_003_enabled = false;
    bool cmf_internal_candidate_enabled = false, cmf_internal_destination_enabled = false;
    bool cmf_internal_compatibility_enabled = false;
    bool membership_shadow_install = false, membership_shadow_enabled = false;
    bool legacy_observer_requested = false;

    [[nodiscard]] const char* cap4_refusal() const noexcept {
        if (!cap4_install || !cap4_enabled) return "both_cap4_gates_required";
        if (!ini_found || !enabled || !cap4_config_valid || !hook_config_valid)
            return "invalid_cap4_configuration";
        if (!target_this_build_only || allow_unknown_build || !fail_closed)
            return "exact_build_fail_closed_required";
        if (hooking_enabled || install_cmf_re_001 || install_cmf_re_003 ||
            install_cmf_internal_candidate || install_cmf_internal_destination ||
            install_cmf_internal_compatibility || cmf_re_001_enabled || cmf_re_003_enabled ||
            cmf_internal_candidate_enabled || cmf_internal_destination_enabled ||
            cmf_internal_compatibility_enabled || membership_shadow_install ||
            membership_shadow_enabled || legacy_observer_requested)
            return "cap4_requires_all_observers_disabled";
        return nullptr;
    }
    [[nodiscard]] const char* refresh_spread_refusal() const noexcept {
        if (!refresh_spread_install || !refresh_spread_enabled) return "both_spread_gates_required";
        if (!refresh_spread_config_valid || (refresh_spread_minutes != 60 && refresh_spread_minutes != 120 && refresh_spread_minutes != 180)) return "only_60_120_or_180_minutes_supported";
        if (cap4_refusal()) return "requires_active_CAP4_and_all_observers_disabled";
        return nullptr;
    }
};
[[nodiscard]] PluginConfig load_config(const std::filesystem::path&) noexcept;
}
