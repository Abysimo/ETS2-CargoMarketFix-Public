#include "logger.h"

#include "build_info.h"

#include <chrono>
#include <ios>
#include <system_error>

namespace cmf {

bool Logger::open(const std::filesystem::path& preferred_path,
                  const std::filesystem::path& fallback_path) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    close_unlocked();

    const auto try_open = [this](const std::filesystem::path& candidate) noexcept {
        try {
            stream_.clear();
            stream_.open(candidate, std::ios::out | std::ios::app);
            if (!stream_.is_open() || !stream_.good()) {
                close_unlocked();
                return false;
            }
            path_ = candidate;
            std::error_code error;
            current_bytes_ = std::filesystem::file_size(candidate, error);
            if (error) {
                current_bytes_ = 0;
            }
            size_limit_reached_ = current_bytes_ >= max_bytes_;
            last_flush_ = std::chrono::steady_clock::now();
            return true;
        } catch (...) {
            close_unlocked();
            return false;
        }
    };

    if (!preferred_path.empty() && try_open(preferred_path)) {
        return true;
    }
    return !fallback_path.empty() && try_open(fallback_path);
}

void Logger::configure(const std::uint32_t flush_interval_ms,
                       const std::uint64_t max_bytes) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    flush_interval_ = std::chrono::milliseconds(flush_interval_ms);
    max_bytes_ = max_bytes;
    size_limit_reached_ = max_bytes_ == 0 || current_bytes_ >= max_bytes_;
}

void Logger::write(const std::string& message) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!stream_.is_open() || size_limit_reached_) {
        return;
    }

    try {
        const std::string line =
            '[' + current_utc_timestamp() + "] " + message + '\n';
        if (line.size() > max_bytes_ - current_bytes_) {
            size_limit_reached_ = true;
            stream_.flush();
            return;
        }

        stream_ << line;
        current_bytes_ += static_cast<std::uint64_t>(line.size());

        const auto now = std::chrono::steady_clock::now();
        if (flush_interval_.count() == 0 ||
            now - last_flush_ >= flush_interval_) {
            stream_.flush();
            last_flush_ = now;
        }
    } catch (...) {
        // Logging must never allow an exception to cross an SDK entry point.
    }
}

void Logger::flush() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!stream_.is_open()) {
        return;
    }

    try {
        stream_.flush();
        last_flush_ = std::chrono::steady_clock::now();
    } catch (...) {
        // Best-effort logging only.
    }
}

void Logger::close() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    close_unlocked();
}

bool Logger::is_open() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return stream_.is_open();
}

std::filesystem::path Logger::path() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        return path_;
    } catch (...) {
        return {};
    }
}

void Logger::close_unlocked() noexcept {
    try {
        if (stream_.is_open()) {
            stream_.flush();
            stream_.close();
        }
        stream_.clear();
        path_.clear();
        current_bytes_ = 0;
        size_limit_reached_ = false;
        last_flush_ = {};
    } catch (...) {
        // Best-effort cleanup only.
    }
}

}  // namespace cmf
