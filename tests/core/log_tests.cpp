#include <dream/core/log.hpp>
#include <dream/core/diagnostic.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
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

    const Diagnostic diagnostic{
        DiagnosticSeverity::error,
        "CORE001",
        "Invalid core state",
        "tests/core/log_tests.cpp"
    };
    require(diagnostic.is_error(), "error diagnostic classification");

    std::cout << "Dream Engine logging and diagnostic tests passed.\n";
    return EXIT_SUCCESS;
}
