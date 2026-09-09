#include "build_info.h"
#include "cap4_patch.h"
#include "refresh_spread.h"
#include "config.h"
#include "logger.h"
#include <scssdk_telemetry.h>
#include <mutex>
#include <string>

namespace {
cmf::Logger logger;
cmf::Cap4Patch cap4;
cmf::RefreshSpread spread;
std::mutex lifecycle_mutex;
bool stop() noexcept {
    const bool spread_stopped = spread.stop(logger);
    const bool cap4_stopped = cap4.stop(logger);
    return spread_stopped && cap4_stopped;
}
}
SCSAPI_RESULT scs_telemetry_init(scs_u32_t version, const scs_telemetry_init_params_t* params) {
    std::lock_guard<std::mutex> lock(lifecycle_mutex);
    try {
        if (!stop()) return SCS_RESULT_generic_error;
        logger.close();
        const auto module = cmf::module_path();
        if (module.empty()) return SCS_RESULT_generic_error;
        const auto directory = module.parent_path();
        logger.open(directory / L"CargoMarketFix.log", {});
        logger.configure(1000, 20ull * 1024 * 1024);
        logger.write(std::string("CargoMarketFix_version = ") + cmf::kPluginVersion);
        logger.write("observers_compiled = false; network_access = none");
        if (version != SCS_TELEMETRY_VERSION_1_00 && version != SCS_TELEMETRY_VERSION_1_01)
            return SCS_RESULT_unsupported;
        if (!params) return SCS_RESULT_invalid_parameter;
        const auto* api = static_cast<const scs_telemetry_init_params_v100_t*>(params);
        if (!api->register_for_event || !api->unregister_from_event ||
            !api->register_for_channel || !api->unregister_from_channel)
            return SCS_RESULT_invalid_parameter;
        const auto config = cmf::load_config(directory / L"CargoMarketFix.ini");
        if (!config.ini_found) {
            logger.write("Missing configuration; no patches installed");
            return SCS_RESULT_generic_error;
        }
        if (!config.enabled) {
            logger.write("Disabled; no patches installed");
            return SCS_RESULT_ok;
        }
        const bool cap_requested = config.cap4_install || config.cap4_enabled;
        const bool spread_requested = config.refresh_spread_install || config.refresh_spread_enabled;
        // Preflight both configurations before either owner may install.
        if ((cap_requested && config.cap4_refusal()) ||
            (spread_requested && config.refresh_spread_refusal())) {
            logger.write("Invalid configuration; no patches installed");
            return SCS_RESULT_generic_error;
        }
        const auto executable = cmf::inspect_executable(cmf::process_executable_path());
        const auto* build = executable.sha256_available ?
            cmf::fix_builds::identify(executable.sha256) : nullptr;
        const bool exact = build != nullptr;
        logger.write(std::string("exact_supported_build = ") + (build ? build->version : "unsupported"));
        if (!exact && (cap_requested || spread_requested)) {
            logger.write("Unsupported executable; no patches installed");
            return SCS_RESULT_generic_error;
        }
        if ((cap_requested && !cap4.start(config, exact, logger, build)) ||
            (spread_requested && !spread.start(config, exact, logger, build))) {
            stop();
            logger.write("Installation failed closed");
            return SCS_RESULT_generic_error;
        }
        logger.write("Initialization complete; observers off");
        logger.flush();
        return SCS_RESULT_ok;
    } catch (...) {
        stop();
        logger.write("Initialization failed; cleanup attempted");
        logger.flush();
        return SCS_RESULT_generic_error;
    }
}
SCSAPI_VOID scs_telemetry_shutdown() {
    std::lock_guard<std::mutex> lock(lifecycle_mutex);
    logger.write(stop() ? "Shutdown clean; patches restored" : "Shutdown containment; pinned resources retained");
    logger.flush();
    logger.close();
}
