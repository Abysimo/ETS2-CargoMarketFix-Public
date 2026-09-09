#include "build_info.h"

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <cstdio>
#include <optional>
#include <string_view>
#include <vector>

namespace cmf {
namespace {

int g_module_anchor = 0;

std::filesystem::path get_module_filename(HMODULE module) noexcept {
    try {
        std::vector<wchar_t> buffer(512);
        for (;;) {
            const DWORD length = GetModuleFileNameW(
                module, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0) {
                return {};
            }
            if (length < buffer.size() - 1) {
                return std::filesystem::path(std::wstring(buffer.data(), length));
            }
            if (buffer.size() >= 32768) {
                return {};
            }
            buffer.resize(buffer.size() * 2);
        }
    } catch (...) {
        return {};
    }
}

std::string system_time_to_iso_utc(const SYSTEMTIME& value) noexcept {
    std::array<char, 32> buffer{};
    const int written = std::snprintf(
        buffer.data(), buffer.size(), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        static_cast<unsigned>(value.wYear),
        static_cast<unsigned>(value.wMonth),
        static_cast<unsigned>(value.wDay),
        static_cast<unsigned>(value.wHour),
        static_cast<unsigned>(value.wMinute),
        static_cast<unsigned>(value.wSecond),
        static_cast<unsigned>(value.wMilliseconds));
    if (written <= 0) {
        return "unavailable";
    }
    return std::string(buffer.data(), static_cast<std::size_t>(written));
}

std::string file_time_to_iso_utc(const FILETIME& value) noexcept {
    SYSTEMTIME system_time{};
    if (!FileTimeToSystemTime(&value, &system_time)) {
        return "unavailable";
    }
    return system_time_to_iso_utc(system_time);
}

std::optional<std::string> sha256_file(const std::filesystem::path& path) noexcept {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    HANDLE file = INVALID_HANDLE_VALUE;

    const auto cleanup = [&]() noexcept {
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }
        if (hash != nullptr) {
            BCryptDestroyHash(hash);
        }
        if (algorithm != nullptr) {
            BCryptCloseAlgorithmProvider(algorithm, 0);
        }
    };

    try {
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
                &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) {
            cleanup();
            return std::nullopt;
        }

        DWORD object_length = 0;
        DWORD hash_length = 0;
        DWORD copied = 0;
        if (!BCRYPT_SUCCESS(BCryptGetProperty(
                algorithm, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&object_length), sizeof(object_length),
                &copied, 0)) ||
            !BCRYPT_SUCCESS(BCryptGetProperty(
                algorithm, BCRYPT_HASH_LENGTH,
                reinterpret_cast<PUCHAR>(&hash_length), sizeof(hash_length),
                &copied, 0))) {
            cleanup();
            return std::nullopt;
        }

        std::vector<UCHAR> hash_object(object_length);
        std::vector<UCHAR> hash_value(hash_length);
        if (!BCRYPT_SUCCESS(BCryptCreateHash(
                algorithm, &hash, hash_object.data(), object_length,
                nullptr, 0, 0))) {
            cleanup();
            return std::nullopt;
        }

        file = CreateFileW(
            path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            cleanup();
            return std::nullopt;
        }

        std::vector<UCHAR> buffer(1024 * 1024);
        for (;;) {
            DWORD bytes_read = 0;
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()),
                          &bytes_read, nullptr)) {
                cleanup();
                return std::nullopt;
            }
            if (bytes_read == 0) {
                break;
            }
            if (!BCRYPT_SUCCESS(BCryptHashData(hash, buffer.data(), bytes_read, 0))) {
                cleanup();
                return std::nullopt;
            }
        }

        if (!BCRYPT_SUCCESS(
                BCryptFinishHash(hash, hash_value.data(), hash_length, 0))) {
            cleanup();
            return std::nullopt;
        }

        static constexpr char kHex[] = "0123456789ABCDEF";
        std::string text;
        text.reserve(hash_value.size() * 2);
        for (const UCHAR byte : hash_value) {
            text.push_back(kHex[(byte >> 4) & 0x0f]);
            text.push_back(kHex[byte & 0x0f]);
        }

        cleanup();
        return text;
    } catch (...) {
        cleanup();
        return std::nullopt;
    }
}

}  // namespace

std::filesystem::path module_path() noexcept {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&g_module_anchor), &module)) {
        return {};
    }
    return get_module_filename(module);
}

std::filesystem::path process_executable_path() noexcept {
    return get_module_filename(nullptr);
}

ExecutableIdentity inspect_executable(const std::filesystem::path& path) noexcept {
    ExecutableIdentity identity;
    try {
        identity.path = path;
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attributes)) {
            ULARGE_INTEGER size{};
            size.HighPart = attributes.nFileSizeHigh;
            size.LowPart = attributes.nFileSizeLow;
            identity.size_bytes = size.QuadPart;
            identity.timestamp_utc =
                file_time_to_iso_utc(attributes.ftLastWriteTime);
            identity.metadata_available = true;
        }

        const auto hash = sha256_file(path);
        if (hash) {
            identity.sha256 = *hash;
            identity.sha256_available = true;
        }
    } catch (...) {
        // Return whatever metadata was gathered before the failure.
    }
    return identity;
}

std::string path_to_utf8(const std::filesystem::path& path) noexcept {
    try {
        const std::wstring text = path.wstring();
        if (text.empty()) {
            return "unavailable";
        }
        const int required = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0) {
            return "unavailable";
        }
        std::string utf8(static_cast<std::size_t>(required), '\0');
        if (WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), utf8.data(), required,
                nullptr, nullptr) <= 0) {
            return "unavailable";
        }
        return utf8;
    } catch (...) {
        return "unavailable";
    }
}

std::string current_utc_timestamp() noexcept {
    SYSTEMTIME now{};
    GetSystemTime(&now);
    return system_time_to_iso_utc(now);
}

std::string format_sdk_version(const std::uint32_t version) noexcept {
    try {
        return std::to_string((version >> 16) & 0xffffu) + "." +
               std::to_string(version & 0xffffu);
    } catch (...) {
        return "unavailable";
    }
}

}  // namespace cmf
