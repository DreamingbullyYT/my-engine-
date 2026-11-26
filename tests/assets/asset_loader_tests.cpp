#include <dream/assets/asset_loader.hpp>

#include <array>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
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
                   std::size_t max_bytes, dream::platform::FileError& error) const override {
        ++read_count;
        const auto found = files.find(std::string(path));
        if (found == files.end()) {
            error = dream::platform::FileError::NotFound;
            return false;
        }
        if (found->second.size() > max_bytes) {
            error = dream::platform::FileError::TooLarge;
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

struct LoadEventRecorder final {
    std::vector<dream::assets::AssetLoadEventV1> events;
    static void record(void* context,
                       const dream::assets::AssetLoadEventV1& event) {
        if(context) static_cast<LoadEventRecorder*>(context)->events.push_back(event);
    }
};
}

int main() {
    using namespace dream;
    core::JobSystem jobs(2);
    MemoryFileSystem filesystem;
    filesystem.files["assets/hello.txt"] = {
        std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}
    };
    filesystem.files["assets/bad.txt"] = {std::byte{'b'}, std::byte{'a'}, std::byte{'d'}};
    filesystem.files["assets/large.txt"] = {std::byte{'1'}, std::byte{'2'}, std::byte{'3'}};

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

    auto too_large = assets::load_async(
        jobs, filesystem, store, "large", "assets/large.txt", decode_text, 2).get();
    require(too_large.status == assets::AssetLoadStatus::ReadFailed,
            "oversized file is rejected by bounded filesystem read");
    require(too_large.file_error == platform::FileError::TooLarge,
            "oversized file reports the size limit error");

    auto invalid_limit = assets::load_async(
        jobs, filesystem, store, "zero-limit", "assets/hello.txt", decode_text, 0).get();
    require(invalid_limit.status == assets::AssetLoadStatus::InvalidLimit,
            "zero maximum file size is rejected");

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

    auto observed_events = std::make_shared<LoadEventRecorder>();
    assets::AssetLoadObserverV1 observer{observed_events, &LoadEventRecorder::record};
    auto observed_load = assets::load_async(
        jobs, filesystem, store, "text/observed", "assets/hello.txt",
        decode_text, 64u * 1024u * 1024u, {}, observer).get();
    const std::array expected_load_phases{
        assets::AssetLoadPhaseV1::ReadBegin,
        assets::AssetLoadPhaseV1::ReadComplete,
        assets::AssetLoadPhaseV1::DecodeBegin,
        assets::AssetLoadPhaseV1::DecodeComplete,
        assets::AssetLoadPhaseV1::ResourceRegisterBegin,
        assets::AssetLoadPhaseV1::ResourceRegisterComplete
    };
    require(observed_load.loaded() &&
            observed_events->events.size() == expected_load_phases.size(),
            "uncached asset load emits every lifecycle phase");
    for(std::size_t i=0U;i<expected_load_phases.size();++i)
        require(observed_events->events[i].phase == expected_load_phases[i],
                "uncached lifecycle events preserve read/decode/register order");
    require(observed_events->events[1].succeeded &&
            observed_events->events[1].source_bytes == 5U &&
            observed_events->events[5].succeeded &&
            observed_events->events[5].key == "text/observed",
            "asset lifecycle events include owned identity, size and result fields");

    auto duplicate = assets::load_async(
        jobs, filesystem, store, "text/hello", "assets/hello.txt", decode_text).get();
    require(duplicate.status == assets::AssetLoadStatus::StoreRejected,
            "duplicate resource key is rejected");

    using LoadResult = assets::AssetLoadResult<core::Handle<TextAssetTag>>;
    assets::AssetRequestCoordinator<LoadResult> request_coordinator;
    std::promise<void> dedup_decoder_entered;
    std::promise<void> dedup_decoder_release;
    auto dedup_release = dedup_decoder_release.get_future().share();
    std::atomic<int> dedup_decoder_calls{0};
    const auto reads_before_dedup = filesystem.read_count.load();

    auto dedup_first = assets::load_async_coordinated(
        request_coordinator, "text/deduplicated", jobs, filesystem, store,
        "text/deduplicated", "assets/hello.txt",
        [&](std::span<const std::byte> bytes) {
            ++dedup_decoder_calls;
            dedup_decoder_entered.set_value();
            dedup_release.wait();
            return decode_text(bytes);
        });
    dedup_decoder_entered.get_future().wait();

    auto dedup_second = assets::load_async_coordinated(
        request_coordinator, "text/deduplicated", jobs, filesystem, store,
        "text/deduplicated", "assets/hello.txt", decode_text);
    dedup_decoder_release.set_value();
    const auto& dedup_first_result = dedup_first.get();
    const auto& dedup_second_result = dedup_second.get();
    require(dedup_first_result.loaded() && dedup_second_result.loaded(),
            "coordinated callers both receive a successful result");
    require(dedup_first_result.handle == dedup_second_result.handle,
            "coordinated callers receive the same resource handle");
    require(dedup_decoder_calls.load() == 1,
            "coordinated callers execute only the first decoder");
    require(filesystem.read_count.load() == reads_before_dedup + 1,
            "coordinated callers perform only one filesystem read");

    assets::AssetCache<TextAsset, TextAssetTag> cache(16);
    const auto reads_before_cached_load = filesystem.read_count.load();
    auto cached = assets::load_async_cached(
        jobs, filesystem, cache, "text/cached", "assets/hello.txt", decode_text,
        [](const TextAsset& asset) { return asset.value.size(); }).get();
    require(cached.loaded() && cached.status == assets::AssetLoadStatus::Loaded,
            "cached loader decodes and inserts a new asset");
    require(cache.find("text/cached") == cached.handle,
            "cached loader returns the cache-owned handle");
    require(cache.used_bytes() == 5,
            "cached loader accounts the caller-provided size estimate");

    auto cached_events = std::make_shared<LoadEventRecorder>();
    assets::AssetLoadObserverV1 cached_observer{
        cached_events, &LoadEventRecorder::record};
    auto observed_cached = assets::load_async_cached(
        jobs, filesystem, cache, "text/observed-cached", "assets/hello.txt",
        decode_text, [](const TextAsset& asset) { return asset.value.size(); },
        64u * 1024u * 1024u, {}, cached_observer).get();
    const std::array expected_cached_phases{
        assets::AssetLoadPhaseV1::CacheLookup,
        assets::AssetLoadPhaseV1::ReadBegin,
        assets::AssetLoadPhaseV1::ReadComplete,
        assets::AssetLoadPhaseV1::DecodeBegin,
        assets::AssetLoadPhaseV1::DecodeComplete,
        assets::AssetLoadPhaseV1::CacheStoreBegin,
        assets::AssetLoadPhaseV1::CacheStoreComplete
    };
    require(observed_cached.loaded() &&
            cached_events->events.size() == expected_cached_phases.size(),
            "cached asset load emits cache/read/decode/store lifecycle phases");
    for(std::size_t i=0U;i<expected_cached_phases.size();++i)
        require(cached_events->events[i].phase == expected_cached_phases[i],
                "cached lifecycle events preserve stage order");
    require(cached_events->events.back().succeeded &&
            cached_events->events.back().decoded_bytes == 5U,
            "cache store event reports its accepted resource size");

    const auto reads_before_cache_hit = filesystem.read_count.load();
    auto cache_hit = assets::load_async_cached(
        jobs, filesystem, cache, "text/cached", "assets/missing-on-purpose.txt", decode_text,
        [](const TextAsset& asset) { return asset.value.size(); }).get();
    require(cache_hit.loaded() && cache_hit.status == assets::AssetLoadStatus::CacheHit,
            "existing cached key returns its handle without reading the file");
    require(cache_hit.handle == cached.handle,
            "cache hit preserves the existing handle");
    require(filesystem.read_count.load() == reads_before_cache_hit,
            "cache hit avoids additional filesystem I/O");

    auto cache_too_large = assets::load_async_cached(
        jobs, filesystem, cache, "text/too-large", "assets/hello.txt", decode_text,
        [](const TextAsset&) { return std::size_t{17}; }).get();
    require(cache_too_large.status == assets::AssetLoadStatus::CacheRejected,
            "asset larger than cache budget is rejected");
    require(cache_too_large.message.find("byte budget") != std::string::npos,
            "cache rejection explains the byte budget");

    assets::AssetCache<TextAsset, TextAssetTag> coordinated_cache(16);
    assets::AssetRequestCoordinator<LoadResult> cached_request_coordinator;
    std::promise<void> cached_dedup_decoder_entered;
    std::promise<void> cached_dedup_decoder_release;
    auto cached_dedup_release = cached_dedup_decoder_release.get_future().share();
    std::atomic<int> cached_dedup_decoder_calls{0};
    const auto reads_before_cached_dedup = filesystem.read_count.load();

    auto cached_dedup_first = assets::load_async_cached_coordinated(
        cached_request_coordinator, "cache/deduplicated", jobs, filesystem,
        coordinated_cache, "cache/deduplicated", "assets/hello.txt",
        [&](std::span<const std::byte> bytes) {
            ++cached_dedup_decoder_calls;
            cached_dedup_decoder_entered.set_value();
            cached_dedup_release.wait();
            return decode_text(bytes);
        },
        [](const TextAsset& asset) { return asset.value.size(); });
    cached_dedup_decoder_entered.get_future().wait();

    auto cached_dedup_second = assets::load_async_cached_coordinated(
        cached_request_coordinator, "cache/deduplicated", jobs, filesystem,
        coordinated_cache, "cache/deduplicated", "assets/hello.txt", decode_text,
        [](const TextAsset& asset) { return asset.value.size(); });
    cached_dedup_decoder_release.set_value();
    const auto& cached_dedup_first_result = cached_dedup_first.get();
    const auto& cached_dedup_second_result = cached_dedup_second.get();
    require(cached_dedup_first_result.loaded() && cached_dedup_second_result.loaded(),
            "coordinated cached callers both receive a successful result");
    require(cached_dedup_first_result.handle == cached_dedup_second_result.handle,
            "coordinated cached callers receive the same cache handle");
    require(cached_dedup_decoder_calls.load() == 1,
            "coordinated cached callers execute only the first decoder");
    require(filesystem.read_count.load() == reads_before_cached_dedup + 1,
            "coordinated cached callers perform only one filesystem read");

    assets::AssetCancellationSource cancelled_before_start;
    cancelled_before_start.cancel();
    const auto reads_before_cancel = filesystem.read_count.load();
    auto cancelled = assets::load_async(
        jobs, filesystem, store, "cancelled", "assets/hello.txt", decode_text,
        64u * 1024u * 1024u, cancelled_before_start.token()).get();
    require(cancelled.status == assets::AssetLoadStatus::Cancelled,
            "pre-cancelled uncached request is cancelled");
    require(filesystem.read_count.load() == reads_before_cancel,
            "pre-cancelled request performs no filesystem read");

    assets::AssetCancellationSource cancelled_during_decode;
    auto cancelled_after_decode = assets::load_async(
        jobs, filesystem, store, "cancelled-after-decode", "assets/hello.txt",
        [&cancelled_during_decode](std::span<const std::byte> bytes) {
            auto result = decode_text(bytes);
            cancelled_during_decode.cancel();
            return result;
        }, 64u * 1024u * 1024u, cancelled_during_decode.token()).get();
    require(cancelled_after_decode.status == assets::AssetLoadStatus::Cancelled,
            "cancellation after decode prevents resource registration");
    require(!store.find("cancelled-after-decode"),
            "cancelled decoded resource is not inserted into ResourceStore");

    assets::AssetCancellationSource cancelled_cache_hit;
    cancelled_cache_hit.cancel();
    auto cancelled_cached = assets::load_async_cached(
        jobs, filesystem, cache, "text/cached", "assets/hello.txt", decode_text,
        [](const TextAsset& asset) { return asset.value.size(); },
        64u * 1024u * 1024u, cancelled_cache_hit.token()).get();
    require(cancelled_cached.status == assets::AssetLoadStatus::Cancelled,
            "cancelled cached request does not return a cache hit");

    jobs.shutdown();
    std::cout << "Dream Engine asset loader tests passed.\n";
    return EXIT_SUCCESS;
}
