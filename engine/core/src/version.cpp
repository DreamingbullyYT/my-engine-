#include <dream/core/version.hpp>

#include <array>
#include <charconv>

namespace dream::core {

std::string_view version_string() noexcept {
    constexpr Version version = engine_version();
    thread_local std::array<char, 32> buffer{};

    char* current = buffer.data();
    const char* const end = buffer.data() + buffer.size();

    const auto write_number = [&current, end](std::uint32_t value) noexcept {
        const auto result = std::to_chars(current, end, value);
        current = result.ptr;
    };

    write_number(version.major);
    *current++ = '.';
    write_number(version.minor);
    *current++ = '.';
    write_number(version.patch);

    return {buffer.data(), static_cast<std::size_t>(current - buffer.data())};
}

} // namespace dream::core
