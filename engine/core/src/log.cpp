#include <dream/core/log.hpp>

#include <iostream>
#include <utility>

namespace dream::core {
namespace {

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

} // namespace

Logger::Logger()
    : sink_([](const LogRecord& record) {
          std::clog << '[' << level_name(record.level) << ']'
                    << '[' << record.category << "] "
                    << record.message << '\n';
      }) {}

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
    std::string_view message
) const {
    if (level < minimum_level()) {
        return;
    }

    Sink sink;
    {
        std::lock_guard lock(sink_mutex_);
        sink = sink_;
    }

    if (sink) {
        sink(LogRecord{level, std::string(category), std::string(message)});
    }
}

} // namespace dream::core
