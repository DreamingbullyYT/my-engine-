#include <dream/core/log.hpp>
#include <dream/core/diagnostic.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
    using namespace dream::core;

    Logger logger;
    std::vector<LogRecord> records;
    logger.set_sink([&records](const LogRecord& record) {
        records.push_back(record);
    });

    logger.set_minimum_level(LogLevel::warning);
    logger.write(LogLevel::info, "core", "filtered");
    logger.write(LogLevel::warning, "renderer", "device unavailable");
    logger.write(LogLevel::critical, "runtime", "fatal failure");

    require(records.size() == 2, "minimum log level filters lower levels");
    require(records[0].level == LogLevel::warning, "warning level retained");
    require(records[0].category == "renderer", "category retained");
    require(records[0].message == "device unavailable", "message retained");
    require(records[1].level == LogLevel::critical, "critical level retained");
    require(records[0].sequence > 0U, "monotonic sequence assigned");
    require(records[0].timestamp.time_since_epoch().count() > 0,
            "wall-clock timestamp assigned");
    require(records[0].thread_id == std::this_thread::get_id(),
            "origin thread retained");
    require(records[0].line > 0U, "source line retained");
    require(records[0].file.find("log_tests.cpp") != std::string::npos,
            "source file retained");
    require(!records[0].function.empty(), "source function retained");

    const auto formatted = format_log_record(records[0]);
    require(formatted.find("[warning][renderer]") != std::string::npos,
            "formatted output contains level and subsystem");
    require(formatted.find("device unavailable") != std::string::npos,
            "formatted output contains message");
    require(formatted.find("log_tests.cpp:") != std::string::npos,
            "formatted output contains source location");
    require(formatted.find("thread=") != std::string::npos,
            "formatted output contains thread id");

    const Diagnostic diagnostic{
        DiagnosticSeverity::error,
        "CORE001",
        "Invalid core state",
        "tests/core/log_tests.cpp"
    };
    require(diagnostic.is_error(), "error diagnostic classification");

    const auto directory = std::filesystem::temp_directory_path() /
        "dream-engine-structured-log-tests";
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    const auto log_path = directory / "logs" / "engine.log";

    Logger file_logger;
    file_logger.set_sink(make_file_log_sink(
        log_path.string(), FileLogSinkOptions{220U, 2U, true}));
    file_logger.set_minimum_level(LogLevel::trace);
    for (int index = 0; index < 8; ++index) {
        file_logger.write(
            LogLevel::info, "test.rotation",
            "record " + std::to_string(index) +
                " with enough context to exercise bounded log rotation");
    }

    require(std::filesystem::exists(log_path), "active file log created");
    require(std::filesystem::exists(log_path.string() + ".1"),
            "first rotated log retained");
    require(std::filesystem::exists(log_path.string() + ".2"),
            "second rotated log retained");
    require(read_file(log_path).find("record 7") != std::string::npos,
            "active log contains latest record");
    require(read_file(log_path.string() + ".1").find("record") != std::string::npos,
            "rotated log contains previous records");
    std::filesystem::remove_all(directory, ec);

    std::cout << "Dream Engine structured logging and diagnostic tests passed.\n";
    return EXIT_SUCCESS;
}
