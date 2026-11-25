#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace dream::core {

enum class LogLevel : std::uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
    critical
};

struct LogRecord final {
    LogLevel level;
    std::string category;
    std::string message;
};

class Logger final {
public:
    using Sink = std::function<void(const LogRecord&)>;

    Logger();

    void set_sink(Sink sink);
    void set_minimum_level(LogLevel level) noexcept;

    [[nodiscard]] LogLevel minimum_level() const noexcept;

    void write(LogLevel level, std::string_view category, std::string_view message) const;

private:
    std::atomic<LogLevel> minimum_level_{LogLevel::trace};
    mutable std::mutex sink_mutex_;
    Sink sink_;
};

} // namespace dream::core
