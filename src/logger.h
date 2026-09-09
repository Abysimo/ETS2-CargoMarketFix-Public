#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace cmf {

class Logger final {
public:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool open(const std::filesystem::path& preferred_path,
              const std::filesystem::path& fallback_path) noexcept;
    void configure(std::uint32_t flush_interval_ms,
                   std::uint64_t max_bytes) noexcept;
    void write(const std::string& message) noexcept;
    void flush() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] std::filesystem::path path() const noexcept;

private:
    void close_unlocked() noexcept;

    mutable std::mutex mutex_;
    std::ofstream stream_;
    std::filesystem::path path_;
    std::chrono::steady_clock::time_point last_flush_{};
    std::chrono::milliseconds flush_interval_{1000};
    std::uint64_t current_bytes_ = 0;
    std::uint64_t max_bytes_ = 20ull * 1024ull * 1024ull;
    bool size_limit_reached_ = false;
};

}  // namespace cmf
