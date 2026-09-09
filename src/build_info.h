#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace cmf {

inline constexpr char kPluginVersion[] =
    "1.0.0";
inline constexpr char kTargetEts2Version[] = "1.57.2.7";
inline constexpr char kExpectedExecutableSha256[] =
    "06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9";

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
