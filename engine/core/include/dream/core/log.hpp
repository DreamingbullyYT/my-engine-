#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace dream::core {

enum class LogLevel : std::uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
    critical
};

// A snapshot of one log event. Timestamps are wall-clock UTC; sequence is a
// process-local monotonic counter, while thread_id and source identify origin.
struct LogRecord final {
    std::uint64_t sequence{0};
    std::chrono::system_clock::time_point timestamp{};
    LogLevel level{LogLevel::info};
    std::string category;
    std::string message;
    std::string file;
    std::string function;
    std::uint_least32_t line{0};
    std::thread::id thread_id{};
};

struct FileLogSinkOptions final {
    // Zero disables size-based rotation. A single record may exceed this limit.
    std::size_t maximum_file_bytes{8U * 1024U * 1024U};
    // Keep engine.log.1 ... engine.log.N in addition to the active file.
    std::size_t backup_count{3U};
    bool flush_each_record{true};
};

class Logger final {
public:
    using Sink = std::function<void(const LogRecord&)>;

    Logger();

    void set_sink(Sink sink);
    void set_minimum_level(LogLevel level) noexcept;
    [[nodiscard]] LogLevel minimum_level() const noexcept;

    void write(
        LogLevel level,
        std::string_view category,
        std::string_view message,
        const std::source_location& source = std::source_location::current()) const;

private:
    std::atomic<LogLevel> minimum_level_{LogLevel::trace};
    mutable std::mutex sink_mutex_;
    Sink sink_;
};

[[nodiscard]] std::string format_log_record(const LogRecord& record);
[[nodiscard]] Logger::Sink make_console_log_sink();
[[nodiscard]] Logger::Sink make_file_log_sink(
    std::string path,
    FileLogSinkOptions options = {});
[[nodiscard]] Logger::Sink make_android_logcat_sink(
    std::string tag = "DreamEngine");
[[nodiscard]] Logger::Sink make_combined_log_sink(
    std::vector<Logger::Sink> sinks);

} // namespace dream::core
