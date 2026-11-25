#include <dream/dmod/package_reader.hpp>

#include <miniz.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ArchiveFiles = std::vector<std::pair<std::string, std::string>>;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

std::string make_manifest(std::string entry = "scripts/main.dms") {
    return "format_version = 1\n"
           "id = \"test.package\"\n"
           "name = \"Test Package\"\n"
           "version = \"1.0.0\"\n"
           "engine_api_min = \"0.1.0\"\n"
           "entry_script = \"" + entry + "\"\n";
}

std::vector<std::byte> make_zip(const ArchiveFiles& files) {
    mz_zip_archive writer{};
    require(mz_zip_writer_init_heap(&writer, 0, 0) != 0, "ZIP writer initializes");
    for (const auto& [path, contents] : files) {
        require(mz_zip_writer_add_mem(&writer, path.c_str(), contents.data(),
                                      contents.size(), MZ_DEFAULT_COMPRESSION) != 0,
                "ZIP entry added");
    }

    void* output = nullptr;
    std::size_t output_size = 0;
    require(mz_zip_writer_finalize_heap_archive(&writer, &output, &output_size) != 0,
            "ZIP archive finalized");
    require(mz_zip_writer_end(&writer) != 0, "ZIP writer closes");

    std::vector<std::byte> bytes(output_size);
    std::memcpy(bytes.data(), output, output_size);
    mz_free(output);
    return bytes;
}

std::unique_ptr<dream::dmod::PackageReader> open_package(
    const std::vector<std::byte>& bytes,
    std::vector<std::string>& errors,
    dream::dmod::PackageLimits limits = {}) {
    return dream::dmod::PackageReader::open(bytes, errors, limits);
}

} // namespace

int main() {
    using namespace dream::dmod;
    const auto manifest = make_manifest();
    const auto script = std::string("return 42;\n");
    const auto valid_zip = make_zip({
        {"manifest.toml", manifest},
        {"scripts/main.dms", script},
        {"assets/readme.txt", "asset"}
    });

    std::vector<std::string> errors;
    auto package = open_package(valid_zip, errors);
    require(package != nullptr, "valid .dmod ZIP package opens");
    require(errors.empty(), "valid package has no errors");
    require(package->manifest().id == "test.package", "package manifest is parsed");
    require(package->entries().size() == 3, "package entries are indexed");

    const auto script_result = package->read_file("scripts/main.dms");
    require(script_result.success(), "entry script can be read from memory");
    require(std::string(reinterpret_cast<const char*>(script_result.bytes.data()),
                        script_result.bytes.size()) == script,
            "entry script contents match");
    require(!package->read_file("Scripts/main.dms").success(),
            "package reads require exact path casing");

    errors.clear();
    auto traversal_zip = make_zip({
        {"manifest.toml", manifest},
        {"scripts/main.dms", script},
        {"../escape.txt", "no"}
    });
    require(open_package(traversal_zip, errors) == nullptr,
            "path traversal package is rejected");

    errors.clear();
    auto collision_zip = make_zip({
        {"manifest.toml", manifest},
        {"scripts/main.dms", script},
        {"Assets/data.txt", "one"},
        {"assets/data.txt", "two"}
    });
    require(open_package(collision_zip, errors) == nullptr,
            "case-colliding paths are rejected");

    errors.clear();
    auto duplicate_zip = make_zip({
        {"manifest.toml", manifest},
        {"scripts/main.dms", script},
        {"assets/duplicate.txt", "first"},
        {"assets/duplicate.txt", "second"}
    });
    require(open_package(duplicate_zip, errors) == nullptr,
            "duplicate archive paths are rejected");

    errors.clear();
    auto symlink_zip = valid_zip;
    mz_zip_archive symlink_reader{};
    require(mz_zip_reader_init_mem(&symlink_reader, symlink_zip.data(),
                                   symlink_zip.size(), 0) != 0,
            "symlink fixture opens for mutation");
    mz_zip_archive_file_stat symlink_stat{};
    require(mz_zip_reader_file_stat(&symlink_reader, 2, &symlink_stat) != 0,
            "symlink fixture entry metadata is read");
    const auto symlink_central = static_cast<std::size_t>(symlink_stat.m_central_dir_ofs);
    mz_zip_reader_end(&symlink_reader);
    symlink_zip[symlink_central + 5] = std::byte{3};
    constexpr std::uint32_t unix_symlink_mode = 0120777u << 16;
    for (unsigned int i = 0; i < 4; ++i) {
        symlink_zip[symlink_central + 38 + i] =
            static_cast<std::byte>((unix_symlink_mode >> (i * 8)) & 0xffu);
    }
    errors.clear();
    require(open_package(symlink_zip, errors) == nullptr,
            "Unix symbolic links are rejected");

    errors.clear();
    auto unsupported_zip = valid_zip;
    mz_zip_archive unsupported_reader{};
    require(mz_zip_reader_init_mem(&unsupported_reader, unsupported_zip.data(),
                                   unsupported_zip.size(), 0) != 0,
            "unsupported-method fixture opens for mutation");
    mz_zip_archive_file_stat unsupported_stat{};
    require(mz_zip_reader_file_stat(&unsupported_reader, 2, &unsupported_stat) != 0,
            "unsupported-method fixture metadata is read");
    const auto unsupported_local = static_cast<std::size_t>(unsupported_stat.m_local_header_ofs);
    const auto unsupported_central = static_cast<std::size_t>(unsupported_stat.m_central_dir_ofs);
    mz_zip_reader_end(&unsupported_reader);
    unsupported_zip[unsupported_local + 8] = std::byte{12};
    unsupported_zip[unsupported_local + 9] = std::byte{0};
    unsupported_zip[unsupported_central + 10] = std::byte{12};
    unsupported_zip[unsupported_central + 11] = std::byte{0};
    errors.clear();
    require(open_package(unsupported_zip, errors) == nullptr,
            "unsupported compression methods are rejected");

    errors.clear();
    auto missing_entry_zip = make_zip({
        {"manifest.toml", make_manifest("scripts/missing.dms")}
    });
    require(open_package(missing_entry_zip, errors) == nullptr,
            "missing entry script is rejected");

    errors.clear();
    PackageLimits small_file_limit;
    small_file_limit.max_entry_uncompressed_bytes = 4;
    small_file_limit.max_manifest_bytes = 4;
    require(open_package(valid_zip, errors, small_file_limit) == nullptr,
            "per-entry size limit is enforced");

    errors.clear();
    PackageLimits small_archive_limit;
    small_archive_limit.max_archive_bytes = 4;
    require(open_package(valid_zip, errors, small_archive_limit) == nullptr,
            "archive size limit is enforced");

    errors.clear();
    auto ratio_zip = make_zip({
        {"manifest.toml", manifest},
        {"scripts/main.dms", std::string(8192, 'A')}
    });
    PackageLimits ratio_limit;
    ratio_limit.max_compression_ratio = 2;
    require(open_package(ratio_zip, errors, ratio_limit) == nullptr,
            "compression ratio limit is enforced");

    errors.clear();
    PackageLimits small_entry_count;
    small_entry_count.max_entries = 2;
    require(open_package(valid_zip, errors, small_entry_count) == nullptr,
            "entry count limit is enforced");

    errors.clear();
    PackageLimits small_total_limit;
    small_total_limit.max_total_uncompressed_bytes = 4;
    require(open_package(valid_zip, errors, small_total_limit) == nullptr,
            "total decompressed size limit is enforced");

    errors.clear();
    auto malformed_header_zip = valid_zip;
    malformed_header_zip[0] = std::byte{0x00};
    require(open_package(malformed_header_zip, errors) == nullptr,
            "malformed local ZIP header is rejected");

    errors.clear();
    auto corrupted_data_zip = valid_zip;
    mz_zip_archive crc_reader{};
    require(mz_zip_reader_init_mem(&crc_reader, corrupted_data_zip.data(),
                                   corrupted_data_zip.size(), 0) != 0,
            "CRC fixture opens for mutation");
    mz_zip_archive_file_stat crc_stat{};
    require(mz_zip_reader_file_stat(&crc_reader, 1, &crc_stat) != 0,
            "CRC fixture entry metadata is read");
    const auto local_header = static_cast<std::size_t>(crc_stat.m_local_header_ofs);
    const auto read_u16 = [&corrupted_data_zip](std::size_t offset) {
        return static_cast<std::size_t>(
            std::to_integer<unsigned char>(corrupted_data_zip[offset]) |
            (std::to_integer<unsigned char>(corrupted_data_zip[offset + 1]) << 8));
    };
    const auto filename_size = read_u16(local_header + 26);
    const auto extra_size = read_u16(local_header + 28);
    const auto compressed_data = local_header + 30 + filename_size + extra_size;
    mz_zip_reader_end(&crc_reader);
    require(compressed_data < corrupted_data_zip.size(), "CRC fixture data offset is valid");
    corrupted_data_zip[compressed_data] ^= std::byte{0x01};
    errors.clear();
    auto corrupted_package = open_package(corrupted_data_zip, errors);
    require(corrupted_package != nullptr, "valid headers with damaged file data can be indexed");
    require(!corrupted_package->read_file("scripts/main.dms").success(),
            "CRC or decompression errors are reported when reading file data");

    errors.clear();
    const std::vector<std::byte> invalid_zip{std::byte{0x01}, std::byte{0x02}};
    require(open_package(invalid_zip, errors) == nullptr,
            "invalid ZIP bytes are rejected");

    std::cout << "Dream Engine DMod package reader tests passed." << std::endl;
    return EXIT_SUCCESS;
}
