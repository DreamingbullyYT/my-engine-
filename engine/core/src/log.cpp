#include <dream/core/log.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace dream::core {
namespace {

std::atomic<std::uint64_t>& global_sequence() noexcept {
    static std::atomic<std::uint64_t> sequence{0};
    return sequence;
}

std::string_view level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::trace: return "trace";
    case LogLevel::debug: return "debug";
    case LogLevel::info: return "info";
    case LogLevel::warning: return "warning";
    case LogLevel::error: return "error";
    case LogLevel::critical: return "critical";
    }
    return "unknown";
}

#if defined(__ANDROID__)
int android_priority(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::trace:
    case LogLevel::debug: return ANDROID_LOG_DEBUG;
    case LogLevel::info: return ANDROID_LOG_INFO;
    case LogLevel::warning: return ANDROID_LOG_WARN;
    case LogLevel::error: return ANDROID_LOG_ERROR;
    case LogLevel::critical: return ANDROID_LOG_FATAL;
    }
    return ANDROID_LOG_INFO;
}
#endif

struct FileSinkState final {
    std::mutex mutex;
    std::filesystem::path path;
    FileLogSinkOptions options;
};

void rotate_logs(const FileSinkState& state) {
    if (state.options.maximum_file_bytes == 0U ||
        !std::filesystem::exists(state.path)) {
        return;
    }

    std::error_code ec;
    const auto current_size = std::filesystem::file_size(state.path, ec);
    if (ec || current_size < state.options.maximum_file_bytes) return;

    if (state.options.backup_count == 0U) {
        std::filesystem::remove(state.path, ec);
        if (ec) throw std::system_error(ec, "remove old engine log");
        return;
    }

    for (std::size_t index = state.options.backup_count; index > 0U; --index) {
        const auto destination = state.path.string() + "." + std::to_string(index);
        const auto source = index == 1U
            ? state.path
            : std::filesystem::path(state.path.string() + "." + std::to_string(index - 1U));

        ec.clear();
        if (!std::filesystem::exists(source, ec)) continue;
        if (ec) throw std::system_error(ec, "inspect engine log backup");

        ec.clear();
        std::filesystem::remove(destination, ec);
        if (ec) throw std::system_error(ec, "remove engine log backup");

        ec.clear();
        std::filesystem::rename(source, destination, ec);
        if (ec) throw std::system_error(ec, "rotate engine log");
    }
}

} // namespace

Logger::Logger() : sink_(make_console_log_sink()) {}

void Logger::set_sink(Sink sink) {
    std::lock_guard lock(sink_mutex_);
    sink_ = std::move(sink);
}

void Logger::set_minimum_level(LogLevel level) noexcept {
    minimum_level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::minimum_level() const noexcept {
    return minimum_level_.load(std::memory_order_relaxed);
}

void Logger::write(
    LogLevel level,
    std::string_view category,
    std::string_view message,
    const std::source_location& source) const {
    if (level < minimum_level()) return;

    LogRecord record;
    record.sequence = global_sequence().fetch_add(1U, std::memory_order_relaxed) + 1U;
    record.timestamp = std::chrono::system_clock::now();
    record.level = level;
    record.category.assign(category);
    record.message.assign(message);
    record.file.assign(source.file_name());
    record.function.assign(source.function_name());
    record.line = source.line();
    record.thread_id = std::this_thread::get_id();

    Sink sink;
    {
        std::lock_guard lock(sink_mutex_);
        sink = sink_;
    }

    if (!sink) return;
    try {
        sink(record);
    } catch (const std::exception& error) {
        // Logging must not take down the engine because a diagnostic sink failed.
        std::clog << "[logger][error] sink failed: " << error.what() << '\n';
    } catch (...) {
        std::clog << "[logger][error] sink failed with an unknown exception\n";
    }
}

std::string format_log_record(const LogRecord& record) {
    const auto seconds = std::chrono::floor<std::chrono::seconds>(record.timestamp);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        record.timestamp - seconds).count();
    const auto time = std::chrono::system_clock::to_time_t(seconds);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif

    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setw(3) << std::setfill('0') << milliseconds << 'Z'
           << " #" << record.sequence
           << " [" << level_name(record.level) << ']'
           << '[' << (record.category.empty() ? "general" : record.category) << ']'
           << "[thread=" << record.thread_id << ']';

    if (!record.file.empty()) {
        output << '[' << record.file;
        if (record.line != 0U) output << ':' << record.line;
        output << ']';
    }
    if (!record.function.empty()) output << '[' << record.function << ']';
    output << ' ' << record.message;
    return output.str();
}

Logger::Sink make_console_log_sink() {
    return [](const LogRecord& record) {
        std::clog << format_log_record(record) << '\n';
    };
}

Logger::Sink make_file_log_sink(std::string path, FileLogSinkOptions options) {
    if (path.empty()) throw std::invalid_argument("engine log path must not be empty");
    auto state = std::make_shared<FileSinkState>();
    state->path = std::filesystem::path(std::move(path));
    state->options = options;

    return [state = std::move(state)](const LogRecord& record) {
        const std::string line = format_log_record(record) + '\n';
        std::lock_guard lock(state->mutex);

        const auto parent = state->path.parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec) throw std::system_error(ec, "create engine log directory");
        }

        rotate_logs(*state);
        std::ofstream output(state->path, std::ios::binary | std::ios::app);
        if (!output) throw std::runtime_error("could not open engine log file");
        output.write(line.data(), static_cast<std::streamsize>(line.size()));
        if (state->options.flush_each_record) output.flush();
        if (!output) throw std::runtime_error("could not write engine log record");
    };
}

Logger::Sink make_android_logcat_sink(std::string tag) {
#if defined(__ANDROID__)
    if (tag.empty()) tag = "DreamEngine";
    return [tag = std::move(tag)](const LogRecord& record) {
        const std::string line = format_log_record(record);
        __android_log_write(android_priority(record.level), tag.c_str(), line.c_str());
    };
#else
    (void)tag;
    return make_console_log_sink();
#endif
}

Logger::Sink make_combined_log_sink(std::vector<Logger::Sink> sinks) {
    return [sinks = std::move(sinks)](const LogRecord& record) {
        for (const auto& sink : sinks) {
            if (!sink) continue;
            try {
                sink(record);
            } catch (const std::exception& error) {
                std::clog << "[logger][error] one sink failed: " << error.what() << '\n';
            } catch (...) {
                std::clog << "[logger][error] one sink failed with an unknown exception\n";
            }
        }
    };
}

} // namespace dream::core
