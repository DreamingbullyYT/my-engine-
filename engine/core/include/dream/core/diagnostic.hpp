#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace dream::core {

enum class DiagnosticSeverity : std::uint8_t {
    info,
    warning,
    error
};

struct Diagnostic final {
    DiagnosticSeverity severity{DiagnosticSeverity::error};
    std::string code;
    std::string message;
    std::string source;

    [[nodiscard]] bool is_error() const noexcept {
        return severity == DiagnosticSeverity::error;
    }
};

} // namespace dream::core
