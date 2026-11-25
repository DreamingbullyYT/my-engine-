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
    const std::vector<std::byte> invalid_zip{std::byte{0x01}, std::byte{0x02}};
    require(open_package(invalid_zip, errors) == nullptr,
            "invalid ZIP bytes are rejected");

    std::cout << "Dream Engine DMod package reader tests passed." << std::endl;
    return EXIT_SUCCESS;
}
