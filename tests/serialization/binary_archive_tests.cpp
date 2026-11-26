#include <dream/serialization/binary_archive.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
using dream::serialization::ArchiveError;
using dream::serialization::ArchiveLimits;
using dream::serialization::ArchiveVersion;
using dream::serialization::BinaryReader;
using dream::serialization::BinaryWriter;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::vector<std::byte> bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (const auto value : values) result.push_back(static_cast<std::byte>(value));
    return result;
}
}

int main() {
    BinaryWriter writer;
    require(writer.write_header({1, 2}), "archive header writes");
    require(writer.write_u8(0xABU), "u8 writes");
    require(writer.write_u16(0x1234U), "u16 writes");
    require(writer.write_u32(0x12345678U), "u32 writes");
    require(writer.write_u64(0x0123456789ABCDEFULL), "u64 writes");
    require(writer.write_i32(-1234567), "i32 writes");
    require(writer.write_i64(-1234567890123LL), "i64 writes");
    require(writer.write_f32(3.25F), "f32 writes");
    require(writer.write_f64(-123.125), "f64 writes");
    require(writer.write_bool(false) && writer.write_bool(true), "booleans write");
    require(writer.write_string("Dream Engine — こんにちは 🌍"), "UTF-8 string writes");
    const auto payload = bytes({0x00U, 0x7FU, 0x80U, 0xFFU});
    require(writer.write_bytes(payload), "opaque bytes write");
    require(writer.good(), "writer remains valid after successful operations");

    BinaryReader reader(writer.data());
    require(reader.read_header({1, 2}), "versioned header reads");
    std::uint8_t u8 = 0;
    std::uint16_t u16 = 0;
    std::uint32_t u32 = 0;
    std::uint64_t u64 = 0;
    std::int32_t i32 = 0;
    std::int64_t i64 = 0;
    float f32 = 0.0F;
    double f64 = 0.0;
    bool false_value = true;
    bool true_value = false;
    std::string text;
    std::array<std::byte, 4> decoded_bytes{};
    require(reader.read_u8(u8) && u8 == 0xABU, "u8 round-trips");
    require(reader.read_u16(u16) && u16 == 0x1234U, "u16 round-trips little-endian");
    require(reader.read_u32(u32) && u32 == 0x12345678U, "u32 round-trips little-endian");
    require(reader.read_u64(u64) && u64 == 0x0123456789ABCDEFULL,
            "u64 round-trips little-endian");
    require(reader.read_i32(i32) && i32 == -1234567, "i32 round-trips");
    require(reader.read_i64(i64) && i64 == -1234567890123LL, "i64 round-trips");
    require(reader.read_f32(f32) && f32 == 3.25F, "f32 preserves exact representation");
    require(reader.read_f64(f64) && f64 == -123.125, "f64 preserves exact representation");
    require(reader.read_bool(false_value) && !false_value, "false round-trips");
    require(reader.read_bool(true_value) && true_value, "true round-trips");
    require(reader.read_string(text) && text == "Dream Engine — こんにちは 🌍",
            "UTF-8 string round-trips");
    require(reader.read_bytes(decoded_bytes), "opaque bytes read");
    require(decoded_bytes == std::array<std::byte, 4>{
                std::byte{0x00}, std::byte{0x7F}, std::byte{0x80}, std::byte{0xFF}},
            "opaque byte values are preserved");
    require(reader.remaining() == 0U, "reader reports no remaining bytes");

    const auto invalid_utf8 = bytes({0xC0U, 0xAFU});
    require(!dream::serialization::is_valid_utf8(std::string_view(
                reinterpret_cast<const char*>(invalid_utf8.data()), invalid_utf8.size())),
            "overlong UTF-8 is rejected");
    const auto truncated_utf8 = bytes({0xE2U, 0x82U});
    require(!dream::serialization::is_valid_utf8(std::string_view(
                reinterpret_cast<const char*>(truncated_utf8.data()), truncated_utf8.size())),
            "truncated UTF-8 is rejected");
    const auto surrogate_utf8 = bytes({0xEDU, 0xA0U, 0x80U});
    require(!dream::serialization::is_valid_utf8(std::string_view(
                reinterpret_cast<const char*>(surrogate_utf8.data()), surrogate_utf8.size())),
            "UTF-8 surrogate encodings are rejected");

    BinaryWriter malformed_string_writer;
    require(malformed_string_writer.write_u32(2U), "malformed fixture length writes");
    const auto malformed_payload = bytes({0xC0U, 0xAFU});
    require(malformed_string_writer.write_bytes(malformed_payload),
            "malformed fixture bytes write");
    BinaryReader malformed_string_reader(malformed_string_writer.data());
    std::string unchanged = "unchanged";
    require(!malformed_string_reader.read_string(unchanged) &&
            malformed_string_reader.error() == ArchiveError::InvalidUtf8,
            "reader rejects invalid UTF-8 string payload");
    require(unchanged == "unchanged", "failed string read leaves output untouched");

    auto bad_boolean = bytes({0x02U});
    BinaryReader boolean_reader(bad_boolean);
    bool boolean_value = false;
    require(!boolean_reader.read_bool(boolean_value) &&
            boolean_reader.error() == ArchiveError::InvalidBoolean,
            "boolean values other than zero or one are rejected");

    auto truncated_integer = bytes({0x34U, 0x12U, 0x56U});
    BinaryReader truncated_reader(truncated_integer);
    std::uint32_t truncated_value = 0xDEADBEEFU;
    require(!truncated_reader.read_u32(truncated_value) &&
            truncated_reader.error() == ArchiveError::UnexpectedEnd,
            "truncated integer is rejected");
    require(truncated_value == 0xDEADBEEFU,
            "failed primitive read leaves output untouched");

    auto bad_magic = bytes({0x58U, 0x45U, 0x4EU, 0x47U, 1U, 0U, 0U, 0U});
    BinaryReader magic_reader(bad_magic);
    require(!magic_reader.read_header() &&
            magic_reader.error() == ArchiveError::InvalidMagic,
            "invalid archive magic is rejected");

    BinaryWriter future_writer;
    require(future_writer.write_header({2, 0}), "future archive fixture writes");
    BinaryReader future_reader(future_writer.data(), ArchiveLimits{});
    require(!future_reader.read_header({1, 9}) &&
            future_reader.error() == ArchiveError::UnsupportedVersion,
            "unsupported major archive version is rejected");

    BinaryWriter future_minor_writer;
    require(future_minor_writer.write_header({1, 3}), "future minor fixture writes");
    BinaryReader future_minor_reader(future_minor_writer.data());
    require(!future_minor_reader.read_header({1, 2}) &&
            future_minor_reader.error() == ArchiveError::UnsupportedVersion,
            "unsupported future minor archive version is rejected");

    BinaryWriter invalid_version_writer;
    require(!invalid_version_writer.write_header({0, 0}) &&
            invalid_version_writer.error() == ArchiveError::InvalidVersion,
            "zero major archive version is rejected");

    ArchiveLimits small_limits{8U, 4U};
    BinaryWriter bounded_writer(small_limits);
    require(!bounded_writer.write_string("12345") &&
            bounded_writer.error() == ArchiveError::TooLarge,
            "writer enforces per-string size limits");

    BinaryWriter total_bounded_writer(ArchiveLimits{4U, 4U});
    require(!total_bounded_writer.write_u64(1U) &&
            total_bounded_writer.error() == ArchiveError::TooLarge,
            "writer enforces total archive size limits");

    const auto oversized_data = bytes({1U, 2U, 3U, 4U, 5U});
    BinaryReader oversized_reader(oversized_data, ArchiveLimits{4U, 4U});
    require(oversized_reader.error() == ArchiveError::TooLarge,
            "reader rejects archives over the configured total size");

    BinaryWriter invalid_limits_writer(ArchiveLimits{4U, 5U});
    require(invalid_limits_writer.error() == ArchiveError::InvalidLimit,
            "invalid writer limits are rejected");
    BinaryReader invalid_limits_reader({}, ArchiveLimits{4U, 5U});
    require(invalid_limits_reader.error() == ArchiveError::InvalidLimit,
            "invalid reader limits are rejected");

    BinaryWriter sticky_writer(ArchiveLimits{2U, 2U});
    require(!sticky_writer.write_u32(3U), "first size error is reported");
    require(!sticky_writer.write_u8(1U) &&
            sticky_writer.error() == ArchiveError::TooLarge,
            "writer error remains sticky");

    std::cout << "Dream Engine binary archive tests passed.\n";
    return EXIT_SUCCESS;
}
