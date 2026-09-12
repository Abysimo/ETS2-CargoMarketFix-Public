#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include "fix_builds.h"

namespace cmf {

#ifdef CMF_RELEASE_VERSION
inline constexpr char kPluginVersion[] = CMF_RELEASE_VERSION;
inline constexpr const char* kTargetEts2Version = fix_builds::supported[0]->version;
inline constexpr const char* kExpectedExecutableSha256 = fix_builds::supported[0]->sha256;
#else
inline constexpr char kPluginVersion[] = "1.4.0";
inline constexpr char kTargetEts2Version[] = "1.60.1.7";
inline constexpr char kExpectedExecutableSha256[] =
    "B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8";
#endif

struct ExecutableIdentity final {
    std::filesystem::path path;
    bool metadata_available = false;
    std::uint64_t size_bytes = 0;
    std::string timestamp_utc;
    bool sha256_available = false;
    std::string sha256;
};

[[nodiscard]] std::filesystem::path module_path() noexcept;
[[nodiscard]] std::filesystem::path process_executable_path() noexcept;
[[nodiscard]] ExecutableIdentity inspect_executable(
    const std::filesystem::path& path) noexcept;
[[nodiscard]] std::string path_to_utf8(const std::filesystem::path& path) noexcept;
[[nodiscard]] std::string current_utc_timestamp() noexcept;
[[nodiscard]] std::string format_sdk_version(std::uint32_t version) noexcept;

}  // namespace cmf
