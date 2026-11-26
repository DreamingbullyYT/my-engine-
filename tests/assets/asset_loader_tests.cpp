#include <dream/assets/asset_loader.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class MemoryFileSystem final : public dream::platform::IFileSystem {
public:
    bool read_file(std::string_view path, std::vector<std::byte>& output,
                   dream::platform::FileError& error) const override {
        ++read_count;
        const auto found = files.find(std::string(path));
        if (found == files.end()) {
            error = dream::platform::FileError::NotFound;
            return false;
        }
        output = found->second;
        error = dream::platform::FileError::None;
        return true;
    }

    bool write_file(std::string_view, std::span<const std::byte>,
                    dream::platform::FileError& error) override {
        error = dream::platform::FileError::Unsupported;
        return false;
    }
    bool exists(std::string_view, bool& result,
                dream::platform::FileError& error) const override {
        result = false;
        error = dream::platform::FileError::Unsupported;
        return false;
    }
    bool create_directories(std::string_view,
                            dream::platform::FileError& error) override {
        error = dream::platform::FileError::Unsupported;
        return false;
    }
    bool remove_file(std::string_view,
                     dream::platform::FileError& error) override {
        error = dream::platform::FileError::Unsupported;
        return false;
    }
    bool list_directory(std::string_view, std::vector<dream::platform::DirectoryEntry>&,
                        dream::platform::FileError& error) const override {
        error = dream::platform::FileError::Unsupported;
        return false;
    }

    mutable std::atomic<int> read_count{0};
    std::map<std::string, std::vector<std::byte>> files;
};

struct TextAssetTag;
struct TextAsset final {
    explicit TextAsset(std::string value) : value(std::move(value)) {}
    std::string value;
};

auto decode_text(std::span<const std::byte> bytes) {
    std::string value;
    value.reserve(bytes.size());
    for (const auto byte : bytes) value.push_back(static_cast<char>(byte));
    if (value == "bad") {
        return dream::assets::AssetDecodeResult<TextAsset>::failure("invalid text asset");
    }
    return dream::assets::AssetDecodeResult<TextAsset>::success(TextAsset(std::move(value)));
}
}

int main() {
    using namespace dream;
    core::JobSystem jobs(2);
    MemoryFileSystem filesystem;
    filesystem.files["assets/hello.txt"] = {
        std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}
    };
    filesystem.files["assets/bad.txt"] = {std::byte{'b'}, std::byte{'a'}, std::byte{'d'}};

    core::ResourceStore<TextAsset, TextAssetTag> store;

    auto invalid_path = assets::load_async(
        jobs, filesystem, store, "invalid-path", "../outside.txt", decode_text).get();
    require(invalid_path.status == assets::AssetLoadStatus::InvalidPath,
            "path traversal is rejected before filesystem access");
    require(filesystem.read_count.load() == 0, "invalid path does not reach filesystem");

    auto missing = assets::load_async(
        jobs, filesystem, store, "missing", "assets/missing.txt", decode_text).get();
    require(missing.status == assets::AssetLoadStatus::ReadFailed,
            "missing file reports read failure");
    require(missing.file_error == platform::FileError::NotFound,
            "filesystem error is preserved");

    auto decode_failed = assets::load_async(
        jobs, filesystem, store, "bad", "assets/bad.txt", decode_text).get();
    require(decode_failed.status == assets::AssetLoadStatus::DecodeFailed,
            "decoder failure is reported");
    require(decode_failed.message == "invalid text asset",
            "decoder diagnostic is preserved");

    auto invalid_key = assets::load_async(
        jobs, filesystem, store, "", "assets/hello.txt", decode_text).get();
    require(invalid_key.status == assets::AssetLoadStatus::InvalidKey,
            "empty asset key is rejected");

    auto loaded = assets::load_async(
        jobs, filesystem, store, "text/hello", "assets/hello.txt", decode_text).get();
    require(loaded.loaded(), "valid asset is loaded asynchronously");
    require(store.find("text/hello") == loaded.handle,
            "loaded resource is registered under its key");

    bool content_matches = false;
    require(store.with_resource(loaded.handle, [&](TextAsset& asset) {
        content_matches = asset.value == "hello";
    }), "loaded asset can be accessed through ResourceStore");
    require(content_matches, "decoder output is stored intact");

    auto duplicate = assets::load_async(
        jobs, filesystem, store, "text/hello", "assets/hello.txt", decode_text).get();
    require(duplicate.status == assets::AssetLoadStatus::StoreRejected,
            "duplicate resource key is rejected");

    jobs.shutdown();
    std::cout << "Dream Engine asset loader tests passed.\n";
    return EXIT_SUCCESS;
}
