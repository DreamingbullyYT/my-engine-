#include <dream/assets/asset_loader.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace {

class BlockingFileSystem final : public dream::platform::IFileSystem {
public:
    bool read_file(std::string_view, std::vector<std::byte>& output,
                   std::size_t, dream::platform::FileError& error) const override {
        std::unique_lock lock(mutex_);
        entered_ = true;
        changed_.notify_all();
        changed_.wait(lock, [this] { return released_; });
        output = {std::byte{0x2A}};
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
        error = dream::platform::FileError::None;
        return true;
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

    bool list_directory(std::string_view,
        std::vector<dream::platform::DirectoryEntry>& output,
        dream::platform::FileError& error) const override {
        output.clear();
        error = dream::platform::FileError::None;
        return true;
    }

    void wait_until_read_started() const {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [this] { return entered_; });
    }

    void release_read() const {
        std::lock_guard lock(mutex_);
        released_ = true;
        changed_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable changed_;
    mutable bool entered_{false};
    mutable bool released_{false};
};

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct AssetTag;

} // namespace

int main() {
    using namespace dream;
    core::JobSystem jobs(1);
    BlockingFileSystem filesystem;
    core::ResourceStore<int, AssetTag> store;

    assets::AssetLoadScope scope;
    auto future = assets::load_async<int, AssetTag>(
        jobs, filesystem, store, "test/blocked", "asset.bin",
        [](std::span<const std::byte>) {
            return assets::AssetDecodeResult<int>::success(42);
        },
        1024, scope.acquire_token());

    filesystem.wait_until_read_started();
    require(scope.active_count() == 1, "submitted work holds one scope lease");

    std::atomic<bool> shutdown_finished{false};
    std::thread shutdown([&] {
        scope.shutdown_and_wait();
        shutdown_finished.store(true, std::memory_order_release);
    });

    while (!scope.is_cancelled()) std::this_thread::yield();
    require(scope.is_shutting_down(), "shutdown closes admission before waiting");
    require(!shutdown_finished.load(std::memory_order_acquire),
            "shutdown waits while a borrowed filesystem call is active");

    filesystem.release_read();
    shutdown.join();

    const auto result = future.get();
    require(shutdown_finished.load(std::memory_order_acquire),
            "shutdown returns after the task releases its lease");
    require(result.status == assets::AssetLoadStatus::Cancelled,
            "a task cancelled during I/O does not continue to decode/register");
    require(store.size() == 0,
            "cancelled task does not publish an asset into the borrowed store");
    require(scope.active_count() == 0,
            "completed tasks release all scope leases");

    auto after_shutdown = scope.acquire_token();
    require(after_shutdown.is_cancelled(),
            "new work acquired after shutdown is immediately cancelled");

    jobs.wait_idle();
    std::cout << "Dream Engine asset load scope tests passed.\n";
    return EXIT_SUCCESS;
}
