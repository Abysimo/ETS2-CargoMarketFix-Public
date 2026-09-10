#include <windows.h>
#include <scssdk_telemetry.h>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
static unsigned cases = 0;
static void check(bool ok, const char* name) {
    if (!ok) throw std::runtime_error(name);
    ++cases; std::printf("PASS %s\n", name);
}
static scs_result_t SCSAPIFUNC event_register(scs_event_t, scs_telemetry_event_callback_t, scs_context_t) {
    throw std::runtime_error("observer registration must not run");
}
static scs_result_t SCSAPIFUNC event_unregister(scs_event_t) { return SCS_RESULT_ok; }
static scs_result_t SCSAPIFUNC channel_register(scs_string_t, scs_u32_t, scs_value_type_t,
    scs_u32_t, scs_telemetry_channel_callback_t, scs_context_t) {
    throw std::runtime_error("observer registration must not run");
}
static scs_result_t SCSAPIFUNC channel_unregister(scs_string_t, scs_u32_t, scs_value_type_t) { return SCS_RESULT_ok; }
int main(int argc, char** argv) {
    try {
        if (argc != 2) return 2;
        const auto dll = std::filesystem::path(argv[1]);
        auto module = LoadLibraryW(dll.c_str());
        check(module != nullptr, "H01_load_release_DLL_in_authored_host");
        using Init = scs_result_t (SCSAPIFUNC *)(scs_u32_t, const scs_telemetry_init_params_t*);
        using Shutdown = void (SCSAPIFUNC *)();
        auto init = reinterpret_cast<Init>(GetProcAddress(module, "scs_telemetry_init"));
        auto shutdown = reinterpret_cast<Shutdown>(GetProcAddress(module, "scs_telemetry_shutdown"));
        check(init && shutdown, "H02_exports_callable");
        check(init(0, nullptr) == SCS_RESULT_unsupported, "H03_unsupported_SDK");
        check(init(SCS_TELEMETRY_VERSION_1_00, nullptr) == SCS_RESULT_invalid_parameter, "H04_null_API");
        scs_telemetry_init_params_v100_t api{};
        check(init(SCS_TELEMETRY_VERSION_1_00, &api) == SCS_RESULT_invalid_parameter, "H05_incomplete_API");
        api.register_for_event = event_register; api.unregister_from_event = event_unregister;
        api.register_for_channel = channel_register; api.unregister_from_channel = channel_unregister;
        check(init(SCS_TELEMETRY_VERSION_1_00, &api) == SCS_RESULT_ok,
              "H06_default_disabled_unknown_host_no_patches_or_observers");
        shutdown(); shutdown();
        check(FreeLibrary(module) != 0 && GetModuleHandleW(dll.filename().c_str()) == nullptr,
              "H07_no_PIN_on_unknown_executable");
        std::printf("TOTAL %u host cases\n", cases); return 0;
    } catch (const std::exception& e) {
        std::printf("FAIL host: %s\n", e.what()); return 1;
    }
}
