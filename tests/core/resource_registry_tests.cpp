#include <dream/core/resource_registry.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct TextureTag;
struct BufferTag;
struct ThrowingTag;
struct TrackedTag;
struct BlockingTag;
struct Resource final {
    explicit Resource(int initial) : value(initial) {}
    int value;
};
struct ThrowingResource final {
    ThrowingResource() { throw std::runtime_error("construction failed"); }
};
struct TrackedResource final {
    explicit TrackedResource(int& destructions) : destructions(destructions) {}
    ~TrackedResource() { ++destructions; }
    int& destructions;
};
struct BlockingResource final {
    explicit BlockingResource(std::atomic<bool>& destroyed) : destroyed(destroyed) {}
    ~BlockingResource() { destroyed.store(true, std::memory_order_release); }
    std::atomic<bool>& destroyed;
};
}

int main() {
    using dream::core::ResourceRegistry;
    static_assert(!std::is_same_v<dream::core::Handle<TextureTag>,
                                  dream::core::Handle<BufferTag>>);

    ResourceRegistry<Resource, TextureTag> textures;
    const auto missing = dream::core::Handle<TextureTag>{};
    require(!textures.contains(missing), "default handle is absent");
    require(!textures.destroy(missing), "default handle cannot be destroyed");

    const auto first = textures.emplace(17);
    require(static_cast<bool>(first), "resource receives a handle");
    require(textures.size() == 1, "registry counts live resources");
    require(textures.contains(first), "new resource is present");

    int observed = 0;
    require(textures.with_resource(first, [&](Resource& resource) {
        observed = resource.value;
        resource.value = 42;
    }), "live resource can be accessed");
    require(observed == 17, "callback observes constructed resource");

    require(textures.destroy(first), "resource can be destroyed");
    require(!textures.contains(first), "destroyed handle is absent");
    require(!textures.with_resource(first, [](Resource&) {}),
            "stale resource cannot be accessed");
    require(!textures.destroy(first), "double destruction is rejected");
    require(textures.size() == 0, "destroy updates live resource count");

    const auto second = textures.emplace(99);
    require(second.index() == first.index(), "released slot is reused");
    require(second.generation() != first.generation(), "reused slot gets new generation");
    require(!textures.contains(first), "old handle remains stale");
    require(textures.contains(second), "new resource is present");

    ResourceRegistry<ThrowingResource, ThrowingTag> throwing;
    try {
        (void)throwing.emplace();
        require(false, "throwing constructor propagates its exception");
    } catch (const std::runtime_error&) {
        require(throwing.size() == 0, "failed construction does not reserve a resource");
    }

    int destructions = 0;
    {
        ResourceRegistry<TrackedResource, TrackedTag> tracked;
        const auto tracked_handle = tracked.emplace(destructions);
        require(tracked.contains(tracked_handle), "tracked resource is registered");
        require(tracked.destroy(tracked_handle), "tracked resource can be destroyed");
        require(destructions == 1, "destroy releases the object exactly once");
    }
    require(destructions == 1, "registry destruction does not double-destroy released objects");

    // A destroy operation must wait until an active callback releases its
    // shared lifetime lock; the object must remain alive throughout the callback.
    std::atomic<bool> blocking_destroyed{false};
    ResourceRegistry<BlockingResource, BlockingTag> blocking;
    const auto blocking_handle = blocking.emplace(blocking_destroyed);
    std::mutex gate_mutex;
    std::condition_variable gate;
    bool callback_entered = false;
    bool allow_callback_exit = false;
    bool destroy_attempted = false;
    bool callback_result = false;
    bool destroy_result = false;

    std::thread accessor([&] {
        callback_result = blocking.with_resource(blocking_handle, [&](BlockingResource&) {
            std::unique_lock lock(gate_mutex);
            callback_entered = true;
            gate.notify_all();
            gate.wait(lock, [&] { return allow_callback_exit; });
        });
    });

    {
        std::unique_lock lock(gate_mutex);
        gate.wait(lock, [&] { return callback_entered; });
    }

    std::thread destroyer([&] {
        {
            std::lock_guard lock(gate_mutex);
            destroy_attempted = true;
        }
        gate.notify_all();
        destroy_result = blocking.destroy(blocking_handle);
    });

    {
        std::unique_lock lock(gate_mutex);
        gate.wait(lock, [&] { return destroy_attempted; });
        require(!blocking_destroyed.load(std::memory_order_acquire),
                "resource remains alive while callback is active");
        allow_callback_exit = true;
    }
    gate.notify_all();
    accessor.join();
    destroyer.join();
    require(callback_result, "concurrent callback completed successfully");
    require(destroy_result, "destroy succeeds after callback releases its lock");
    require(blocking_destroyed.load(std::memory_order_acquire),
            "resource is destroyed after callback completion");

    ResourceRegistry<std::string, BufferTag> buffers;
    const auto text = buffers.emplace("mesh data");
    require(buffers.with_resource(text, [&](std::string& value) {
        require(value == "mesh data", "move-aware resource construction");
    }), "independent typed registry works");

    std::cout << "Dream Engine resource registry tests passed.\n";
    return EXIT_SUCCESS;
}
