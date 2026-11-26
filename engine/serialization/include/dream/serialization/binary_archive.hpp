#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dream::serialization {

struct ArchiveVersion final {
    std::uint16_t major{1};
    std::uint16_t minor{0};
};

struct ArchiveLimits final {
    std::size_t maximum_bytes{64U * 1024U * 1024U};
    std::size_t maximum_string_bytes{1U * 1024U * 1024U};
};

enum class ArchiveError : std::uint8_t {
    None,
    InvalidLimit,
    TooLarge,
    UnexpectedEnd,
    InvalidUtf8,
    InvalidBoolean,
    InvalidMagic,
    InvalidVersion,
    UnsupportedVersion
};

[[nodiscard]] inline bool is_valid_utf8(std::string_view text) noexcept {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    std::size_t index = 0;
    while (index < text.size()) {
        const unsigned char first = bytes[index];
        if (first <= 0x7FU) {
            ++index;
            continue;
        }

        std::size_t continuation_count = 0;
        unsigned char second_min = 0x80U;
        unsigned char second_max = 0xBFU;

        if (first >= 0xC2U && first <= 0xDFU) {
            continuation_count = 1;
        } else if (first >= 0xE0U && first <= 0xEFU) {
            continuation_count = 2;
            if (first == 0xE0U) second_min = 0xA0U;
            if (first == 0xEDU) second_max = 0x9FU;
        } else if (first >= 0xF0U && first <= 0xF4U) {
            continuation_count = 3;
            if (first == 0xF0U) second_min = 0x90U;
            if (first == 0xF4U) second_max = 0x8FU;
        } else {
            return false;
        }

        if (continuation_count > text.size() - index - 1U) return false;
        const unsigned char second = bytes[index + 1U];
        if (second < second_min || second > second_max) return false;
        for (std::size_t offset = 2; offset <= continuation_count; ++offset) {
            const unsigned char value = bytes[index + offset];
            if (value < 0x80U || value > 0xBFU) return false;
        }
        index += continuation_count + 1U;
    }
    return true;
}

class BinaryWriter final {
public:
    explicit BinaryWriter(ArchiveLimits limits = {}) : limits_(limits) {
        if (limits_.maximum_bytes == 0U ||
            limits_.maximum_string_bytes > limits_.maximum_bytes ||
            limits_.maximum_string_bytes > std::numeric_limits<std::uint32_t>::max()) {
            error_ = ArchiveError::InvalidLimit;
        }
    }

    [[nodiscard]] ArchiveError error() const noexcept { return error_; }
    [[nodiscard]] bool good() const noexcept { return error_ == ArchiveError::None; }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    [[nodiscard]] const std::vector<std::byte>& data() const noexcept { return data_; }
    [[nodiscard]] std::vector<std::byte> take() && noexcept { return std::move(data_); }

    bool write_header(ArchiveVersion version = {}) {
        if (!good()) return false;
        if (version.major == 0U) return fail(ArchiveError::InvalidVersion);

        constexpr std::array<std::byte, 4> magic{
            std::byte{'D'}, std::byte{'E'}, std::byte{'N'}, std::byte{'G'}};
        constexpr std::size_t header_size = 8U;
        if (!can_append(header_size)) return false;

        // Reserve before mutating so allocation failure cannot leave a partial header.
        data_.reserve(data_.size() + header_size);
        append_unchecked(magic);
        write_unsigned_unchecked(version.major);
        write_unsigned_unchecked(version.minor);
        return true;
    }

    bool write_u8(std::uint8_t value) { return write_unsigned(value); }
    bool write_u16(std::uint16_t value) { return write_unsigned(value); }
    bool write_u32(std::uint32_t value) { return write_unsigned(value); }
    bool write_u64(std::uint64_t value) { return write_unsigned(value); }
    bool write_i32(std::int32_t value) {
        return write_unsigned(std::bit_cast<std::uint32_t>(value));
    }
    bool write_i64(std::int64_t value) {
        return write_unsigned(std::bit_cast<std::uint64_t>(value));
    }
    bool write_f32(float value) {
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        return write_unsigned(std::bit_cast<std::uint32_t>(value));
    }
    bool write_f64(double value) {
        static_assert(sizeof(double) == sizeof(std::uint64_t));
        return write_unsigned(std::bit_cast<std::uint64_t>(value));
    }
    bool write_bool(bool value) { return write_u8(value ? 1U : 0U); }

    bool write_bytes(std::span<const std::byte> bytes) {
        if (!good()) return false;
        if (!can_append(bytes.size())) return false;
        if (bytes.empty()) return true;
        data_.reserve(data_.size() + bytes.size());
        append_unchecked(bytes);
        return true;
    }

    bool write_string(std::string_view value) {
        if (!good()) return false;
        if (value.size() > limits_.maximum_string_bytes ||
            value.size() > std::numeric_limits<std::uint32_t>::max()) {
            return fail(ArchiveError::TooLarge);
        }
        if (!is_valid_utf8(value)) return fail(ArchiveError::InvalidUtf8);

        constexpr std::size_t length_size = sizeof(std::uint32_t);
        if (value.size() > std::numeric_limits<std::size_t>::max() - length_size)
            return fail(ArchiveError::TooLarge);
        const std::size_t required = length_size + value.size();
        if (!can_append(required)) return false;

        // One reserve and no throwing operation after the size-prefix is appended.
        data_.reserve(data_.size() + required);
        write_unsigned_unchecked(static_cast<std::uint32_t>(value.size()));
        const auto* first = reinterpret_cast<const std::byte*>(value.data());
        append_unchecked(std::span<const std::byte>{first, value.size()});
        return true;
    }

private:
    bool fail(ArchiveError error) noexcept {
        if (error_ == ArchiveError::None) error_ = error;
        return false;
    }

    bool can_append(std::size_t amount) noexcept {
        if (!good()) return false;
        if (amount > limits_.maximum_bytes - data_.size())
            return fail(ArchiveError::TooLarge);
        return true;
    }

    template <typename Unsigned>
    bool write_unsigned(Unsigned value) {
        static_assert(std::is_unsigned_v<Unsigned>);
        if (!can_append(sizeof(Unsigned))) return false;
        data_.reserve(data_.size() + sizeof(Unsigned));
        write_unsigned_unchecked(value);
        return true;
    }

    template <typename Unsigned>
    void write_unsigned_unchecked(Unsigned value) noexcept {
        static_assert(std::is_unsigned_v<Unsigned>);
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
            data_.push_back(static_cast<std::byte>(
                static_cast<std::uint8_t>(value & static_cast<Unsigned>(0xFFU))));
            value >>= 8U;
        }
    }

    void append_unchecked(std::span<const std::byte> bytes) noexcept {
        data_.insert(data_.end(), bytes.begin(), bytes.end());
    }

    ArchiveLimits limits_;
    ArchiveError error_{ArchiveError::None};
    std::vector<std::byte> data_;
};

class BinaryReader final {
public:
    explicit BinaryReader(
        std::span<const std::byte> data, ArchiveLimits limits = {}) noexcept
        : data_(data), limits_(limits) {
        if (limits_.maximum_bytes == 0U ||
            limits_.maximum_string_bytes > limits_.maximum_bytes ||
            limits_.maximum_string_bytes > std::numeric_limits<std::uint32_t>::max()) {
            error_ = ArchiveError::InvalidLimit;
        } else if (data_.size() > limits_.maximum_bytes) {
            error_ = ArchiveError::TooLarge;
        }
    }

    [[nodiscard]] ArchiveError error() const noexcept { return error_; }
    [[nodiscard]] bool good() const noexcept { return error_ == ArchiveError::None; }
    [[nodiscard]] std::size_t position() const noexcept { return position_; }
    [[nodiscard]] std::size_t remaining() const noexcept {
        return position_ <= data_.size() ? data_.size() - position_ : 0U;
    }

    bool read_header(ArchiveVersion supported = {}) noexcept {
        if (!good()) return false;
        std::array<std::byte, 4> magic{};
        if (!read_bytes(magic)) return false;
        constexpr std::array<std::byte, 4> expected{
            std::byte{'D'}, std::byte{'E'}, std::byte{'N'}, std::byte{'G'}};
        if (magic != expected) return fail(ArchiveError::InvalidMagic);

        std::uint16_t major = 0;
        std::uint16_t minor = 0;
        if (!read_u16(major) || !read_u16(minor)) return false;
        if (major == 0U) return fail(ArchiveError::InvalidVersion);
        if (major != supported.major || minor > supported.minor)
            return fail(ArchiveError::UnsupportedVersion);
        return true;
    }

    bool read_u8(std::uint8_t& value) noexcept { return read_unsigned(value); }
    bool read_u16(std::uint16_t& value) noexcept { return read_unsigned(value); }
    bool read_u32(std::uint32_t& value) noexcept { return read_unsigned(value); }
    bool read_u64(std::uint64_t& value) noexcept { return read_unsigned(value); }

    bool read_i32(std::int32_t& value) noexcept {
        std::uint32_t bits = 0;
        if (!read_unsigned(bits)) return false;
        value = std::bit_cast<std::int32_t>(bits);
        return true;
    }
    bool read_i64(std::int64_t& value) noexcept {
        std::uint64_t bits = 0;
        if (!read_unsigned(bits)) return false;
        value = std::bit_cast<std::int64_t>(bits);
        return true;
    }
    bool read_f32(float& value) noexcept {
        std::uint32_t bits = 0;
        if (!read_unsigned(bits)) return false;
        value = std::bit_cast<float>(bits);
        return true;
    }
    bool read_f64(double& value) noexcept {
        std::uint64_t bits = 0;
        if (!read_unsigned(bits)) return false;
        value = std::bit_cast<double>(bits);
        return true;
    }

    bool read_bool(bool& value) noexcept {
        std::uint8_t encoded = 0;
        if (!read_u8(encoded)) return false;
        if (encoded > 1U) return fail(ArchiveError::InvalidBoolean);
        value = encoded == 1U;
        return true;
    }

    bool read_bytes(std::span<std::byte> output) noexcept {
        if (!good()) return false;
        if (output.size() > remaining()) return fail(ArchiveError::UnexpectedEnd);
        if (!output.empty())
            std::memcpy(output.data(), data_.data() + position_, output.size());
        position_ += output.size();
        return true;
    }

    bool read_string(std::string& output) {
        if (!good()) return false;
        std::uint32_t length = 0;
        if (!read_u32(length)) return false;
        if (length > limits_.maximum_string_bytes)
            return fail(ArchiveError::TooLarge);
        if (length > remaining()) return fail(ArchiveError::UnexpectedEnd);

        const char* text = reinterpret_cast<const char*>(data_.data() + position_);
        const std::string_view view{text, length};
        if (!is_valid_utf8(view)) return fail(ArchiveError::InvalidUtf8);
        output.assign(view);
        position_ += length;
        return true;
    }

private:
    bool fail(ArchiveError error) noexcept {
        if (error_ == ArchiveError::None) error_ = error;
        return false;
    }

    template <typename Unsigned>
    bool read_unsigned(Unsigned& output) noexcept {
        static_assert(std::is_unsigned_v<Unsigned>);
        if (!good()) return false;
        if (sizeof(Unsigned) > remaining()) return fail(ArchiveError::UnexpectedEnd);

        std::uint64_t value = 0;
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
            value |= static_cast<std::uint64_t>(
                std::to_integer<std::uint8_t>(data_[position_ + index])) << (index * 8U);
        }
        output = static_cast<Unsigned>(value);
        position_ += sizeof(Unsigned);
        return true;
    }

    ArchiveLimits limits_;
    std::span<const std::byte> data_;
    std::size_t position_{0};
    ArchiveError error_{ArchiveError::None};
};

} // namespace dream::serialization
