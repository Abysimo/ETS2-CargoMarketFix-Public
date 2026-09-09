#include "build_info.h"
#include "config.h"
#include "cap4_patch.h"
#include "refresh_spread.h"
#include "logger.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

static unsigned cases = 0;
static void check(bool ok, const char* name) {
    if (!ok) throw std::runtime_error(name);
    ++cases; std::printf("PASS %s\n", name);
}
int main(int argc, char** argv) {
    try {
        if (argc != 3) return 2;
        auto c = cmf::load_config(argv[1]);
        check(std::strcmp(cmf::kPluginVersion, "1.0.0") == 0, "R01_release_version");
        check(c.enabled && c.cap4_install && c.cap4_enabled && !c.cap4_refusal(), "R02_release_CAP4_on");
        check(c.refresh_spread_install && c.refresh_spread_enabled && c.refresh_spread_minutes == 60 &&
              !c.refresh_spread_refusal(), "R03_release_spread_on");
        check(!c.hooking_enabled && !c.membership_shadow_install && !c.membership_shadow_enabled &&
              !c.install_cmf_internal_compatibility && !c.cmf_internal_compatibility_enabled,
              "R04_observers_off");
        check(c.target_this_build_only && !c.allow_unknown_build && c.fail_closed, "R05_fixed_build_gates");
        const auto game = cmf::inspect_executable(argv[2]);
        check(game.sha256_available && game.sha256 == cmf::kExpectedExecutableSha256, "R06_exact_game_hash_readonly");
        const auto self = cmf::inspect_executable(cmf::process_executable_path());
        check(self.sha256_available && self.sha256 != cmf::kExpectedExecutableSha256, "R07_unsupported_host_hash");
        cmf::Logger log; cmf::Cap4Patch cap4; cmf::RefreshSpread spread;
        check(!cap4.start(c, false, log) && !spread.start(c, false, log) &&
              !cap4.potentially_live() && !cmf::hook_module_was_pinned(), "R08_unknown_build_no_patch_PIN");
        auto missing = cmf::load_config({});
        check(!missing.ini_found && !missing.cap4_install && !missing.refresh_spread_install, "R09_missing_INI_no_install");
        c.cap4_config_valid = false;
        check(c.cap4_refusal() && c.refresh_spread_refusal(), "R10_invalid_CAP4_blocks_both");
        c = cmf::load_config(argv[1]); c.refresh_spread_minutes = 30;
        check(c.refresh_spread_refusal(), "R11_no_alternative_scheduler");
        c = cmf::load_config(argv[1]); c.target_this_build_only = false;
        check(c.cap4_refusal() && c.refresh_spread_refusal(), "R12_exact_build_cannot_relax");
        check(cmf::cap4::target_rva == 0x006CE618 && cmf::spread::target_rva == 0x003EC647,
              "R13_fixed_patch_targets");
        check(cap4.stop(log) && spread.stop(log), "R14_empty_cleanup");
        std::printf("TOTAL %u release cases\n", cases);
        return 0;
    } catch (const std::exception& e) {
        std::printf("FAIL release: %s\n", e.what()); return 1;
    }
}
