#include <dream/dmod/package_reader.hpp>

#include <dream/dmod/manifest_parser.hpp>

#include <miniz.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dream::dmod {
namespace {

bool valid_utf8(std::string_view text) {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7f) {
            if (first == 0 || first < 0x20 || first == 0x7f) return false;
            ++i;
            continue;
        }

        std::size_t count = 0;
        std::uint32_t codepoint = 0;
        if ((first & 0xe0) == 0xc0) {
            count = 2;
            codepoint = first & 0x1f;
            if (codepoint < 2) return false;
        } else if ((first & 0xf0) == 0xe0) {
            count = 3;
            codepoint = first & 0x0f;
        } else if ((first & 0xf8) == 0xf0) {
            count = 4;
            codepoint = first & 0x07;
        } else {
            return false;
        }

        if (i + count > text.size()) return false;
        for (std::size_t j = 1; j < count; ++j) {
            const auto next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (next & 0x3f);
        }

        if ((count == 3 && codepoint < 0x800) ||
            (count == 4 && codepoint < 0x10000) ||
            codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff)) {
            return false;
        }
        i += count;
    }
    return true;
}

bool normalize_archive_path(
    std::string_view path, bool directory, std::string& lookup_key) {
    if (path.empty() || path.size() > 1024 || !valid_utf8(path)) return false;
    if (path.front() == '/' || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos) return false;

    if (directory) {
        if (path.back() != '/') return false;
        path.remove_suffix(1);
    } else if (path.back() == '/') {
        return false;
    }
    if (path.empty()) return false;

    std::size_t begin = 0;
    while (begin < path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos
            ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == ".." ||
            segment.back() == '.' || segment.back() == ' ') return false;
        begin = end == std::string_view::npos ? path.size() : end + 1;
    }

    lookup_key.assign(path);
    std::transform(lookup_key.begin(), lookup_key.end(), lookup_key.begin(),
        [](unsigned char c) { return static_cast<char>(c < 0x80 ? std::tolower(c) : c); });
    return true;
}

bool exceeds_ratio(std::uint64_t compressed, std::uint64_t uncompressed,
                   std::uint64_t maximum_ratio) {
    if (uncompressed == 0) return false;
    if (compressed == 0) return true;
    if (compressed > std::numeric_limits<std::uint64_t>::max() / maximum_ratio) {
        return false;
    }
    return uncompressed > compressed * maximum_ratio;
}

} // namespace

struct PackageReader::Impl final {
    struct IndexedEntry final {
        PackageEntry public_entry;
        mz_uint archive_index{};
    };

    std::span<const std::byte> archive_bytes;
    PackageLimits limits;
    mz_zip_archive archive{};
    bool archive_initialized{false};
    Manifest manifest;
    std::vector<PackageEntry> entries;
    std::unordered_map<std::string, IndexedEntry> by_path;

    ~Impl() {
        if (archive_initialized) mz_zip_reader_end(&archive);
    }

    PackageFileResult read_index(mz_uint index, std::uint64_t expected_size) const {
        PackageFileResult result;
        if (expected_size > limits.max_entry_uncompressed_bytes ||
            expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            result.error = "Entry exceeds the configured read limit.";
            return result;
        }

        const auto size = static_cast<std::size_t>(expected_size);
        result.bytes.resize(size == 0 ? 1 : size);
        if (!mz_zip_reader_extract_to_mem(
                const_cast<mz_zip_archive*>(&archive), index,
                result.bytes.data(), size, 0)) {
            result.bytes.clear();
            result.error = "ZIP entry decompression or CRC validation failed.";
            return result;
        }
        result.bytes.resize(size);
        return result;
    }
};

PackageReader::PackageReader(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
PackageReader::~PackageReader() = default;

std::unique_ptr<PackageReader> PackageReader::open(
    std::span<const std::byte> archive_bytes,
    std::vector<std::string>& errors,
    PackageLimits limits) {
    const auto fail = [&errors](std::string message) -> std::unique_ptr<PackageReader> {
        errors.push_back(std::move(message));
        return nullptr;
    };

    if (archive_bytes.empty() || archive_bytes.size() > limits.max_archive_bytes) {
        return fail("Archive is empty or exceeds the configured archive size limit.");
    }
    if (limits.max_entries == 0 || limits.max_entry_uncompressed_bytes == 0 ||
        limits.max_total_uncompressed_bytes == 0 || limits.max_compression_ratio == 0 ||
        limits.max_manifest_bytes == 0 ||
        limits.max_manifest_bytes > limits.max_entry_uncompressed_bytes) {
        return fail("Package limits are invalid.");
    }

    auto impl = std::make_unique<Impl>();
    impl->archive_bytes = archive_bytes;
    impl->limits = limits;
    if (!mz_zip_reader_init_mem(&impl->archive, archive_bytes.data(),
                                archive_bytes.size(), 0)) {
        return fail("Input is not a valid supported ZIP archive.");
    }
    impl->archive_initialized = true;

    const auto file_count = mz_zip_reader_get_num_files(&impl->archive);
    if (file_count == 0 || file_count > limits.max_entries) {
        return fail("Archive entry count is zero or exceeds the configured limit.");
    }

    std::uint64_t total_uncompressed = 0;
    impl->entries.reserve(file_count);
    impl->by_path.reserve(file_count);

    for (mz_uint index = 0; index < file_count; ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&impl->archive, index, &stat)) {
            return fail("Could not read ZIP central-directory metadata.");
        }

        const auto filename_capacity = mz_zip_reader_get_filename(
            &impl->archive, index, nullptr, 0);
        if (filename_capacity < 2 || filename_capacity > 1025) {
            return fail("Archive contains an empty or overlong path.");
        }
        std::vector<char> filename(filename_capacity, '\0');
        const auto filename_written = mz_zip_reader_get_filename(
            &impl->archive, index, filename.data(), filename_capacity);
        if (filename_written != filename_capacity) {
            return fail("Could not read a complete ZIP entry path.");
        }
        const std::string path(filename.data(), filename_capacity - 1);

        const auto host_os = static_cast<unsigned char>(stat.m_version_made_by >> 8);
        const auto unix_mode = stat.m_external_attr >> 16;
        const auto unix_type = unix_mode & 0170000u;
        if ((host_os == 0 && (stat.m_external_attr & 0x400u) != 0) ||
            (host_os == 3 && unix_type != 0 &&
             unix_type != 0100000u && unix_type != 0040000u)) {
            return fail("Archive contains a link or unsupported special file.");
        }

        const bool directory = stat.m_is_directory != 0 || unix_type == 0040000u;
        if (directory && stat.m_uncomp_size != 0) {
            return fail("Directory entries must not contain file data.");
        }
        if (!directory && (stat.m_is_encrypted || !stat.m_is_supported)) {
            return fail("Encrypted or unsupported ZIP entries are not allowed.");
        }
        if (!directory && stat.m_method != 0 && stat.m_method != 8) {
            return fail("Only stored and deflate ZIP entries are supported.");
        }

        std::string key;
        if (!normalize_archive_path(path, directory, key)) {
            return fail("Archive contains an unsafe or non-canonical path: " + path);
        }
        if (impl->by_path.contains(key)) {
            return fail("Archive contains duplicate or case-colliding paths: " + path);
        }

        if (!directory) {
            if (stat.m_uncomp_size > limits.max_entry_uncompressed_bytes) {
                return fail("An archive entry exceeds the per-file decompressed size limit.");
            }
            if (exceeds_ratio(stat.m_comp_size, stat.m_uncomp_size,
                              limits.max_compression_ratio)) {
                return fail("An archive entry exceeds the compression-ratio limit.");
            }
            if (stat.m_uncomp_size > limits.max_total_uncompressed_bytes - total_uncompressed) {
                return fail("Archive exceeds the total decompressed size limit.");
            }
            total_uncompressed += stat.m_uncomp_size;
        }

        PackageEntry entry{path, stat.m_comp_size, stat.m_uncomp_size, directory};
        impl->entries.push_back(entry);
        impl->by_path.emplace(std::move(key), Impl::IndexedEntry{entry, index});
    }

    for (const auto& entry : impl->entries) {
        std::string key;
        if (!normalize_archive_path(entry.path, entry.directory, key)) {
            return fail("Archive path changed during validation.");
        }
        auto separator = key.find('/');
        while (separator != std::string::npos) {
            const auto parent = key.substr(0, separator);
            const auto found = impl->by_path.find(parent);
            if (found != impl->by_path.end() && !found->second.public_entry.directory) {
                return fail("A file path is also used as a parent directory.");
            }
            separator = key.find('/', separator + 1);
        }
    }

    const auto manifest_entry = impl->by_path.find("manifest.toml");
    if (manifest_entry == impl->by_path.end() ||
        manifest_entry->second.public_entry.directory ||
        manifest_entry->second.public_entry.path != "manifest.toml") {
        return fail("Package must contain a root-level manifest.toml file.");
    }
    if (manifest_entry->second.public_entry.uncompressed_size > limits.max_manifest_bytes) {
        return fail("manifest.toml exceeds the configured manifest size limit.");
    }

    auto package = std::unique_ptr<PackageReader>(new PackageReader(std::move(impl)));
    const auto manifest_bytes = package->read_file("manifest.toml");
    if (!manifest_bytes.success()) return fail("Could not read manifest.toml: " + manifest_bytes.error);

    if (manifest_bytes.bytes.empty()) return fail("manifest.toml is empty.");
    const std::string manifest_text(
        reinterpret_cast<const char*>(manifest_bytes.bytes.data()), manifest_bytes.bytes.size());
    const auto parsed = parse_manifest(manifest_text);
    if (!parsed.valid()) {
        for (const auto& error : parsed.errors) errors.push_back("manifest.toml: " + error);
        return nullptr;
    }
    package->impl_->manifest = *parsed.manifest;

    std::string entry_key;
    if (!normalize_archive_path(package->impl_->manifest.entry_script, false, entry_key)) {
        return fail("Manifest entry_script is not a safe package path.");
    }
    const auto entry_script = package->impl_->by_path.find(entry_key);
    if (entry_script == package->impl_->by_path.end() ||
        entry_script->second.public_entry.directory ||
        entry_script->second.public_entry.path != package->impl_->manifest.entry_script) {
        return fail("Manifest entry_script does not refer to a package file.");
    }

    return package;
}

const Manifest& PackageReader::manifest() const noexcept {
    return impl_->manifest;
}

const std::vector<PackageEntry>& PackageReader::entries() const noexcept {
    return impl_->entries;
}

PackageFileResult PackageReader::read_file(std::string_view path) const {
    PackageFileResult result;
    std::string key;
    if (!normalize_archive_path(path, false, key)) {
        result.error = "Requested path is not a safe relative package path.";
        return result;
    }
    const auto found = impl_->by_path.find(key);
    if (found == impl_->by_path.end() || found->second.public_entry.directory ||
        found->second.public_entry.path != path) {
        result.error = "Package file does not exist.";
        return result;
    }
    return impl_->read_index(found->second.archive_index,
                             found->second.public_entry.uncompressed_size);
}

} // namespace dream::dmod
