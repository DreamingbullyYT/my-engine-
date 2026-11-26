#pragma once

#include <dream/core/handle.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dream::world {

struct EntityTag;
using Entity = core::Handle<EntityTag>;

// A thread-safe entity/component store. Structural changes are serialized.
// Component callbacks execute while the world's lock is held and must not
// re-enter this World. Read callbacks receive const components; mutation
// callbacks take the exclusive lock and receive mutable references.
class World final {
private:
    struct IComponentPool {
        virtual ~IComponentPool() = default;
        virtual void erase(Entity entity) noexcept = 0;
    };

    template <typename Component>
    class ComponentPool final : public IComponentPool {
    public:
        template <typename... Args>
        bool emplace(Entity entity, Args&&... args) {
            const auto found = values_.find(entity.index());
            if (found != values_.end()) {
                if (found->second.generation == entity.generation()) return false;
                values_.erase(found);
            }
            values_.try_emplace(
                entity.index(), entity.generation(), std::forward<Args>(args)...);
            return true;
        }

        [[nodiscard]] const Component* find(Entity entity) const noexcept {
            const auto found = values_.find(entity.index());
            if (found == values_.end() ||
                found->second.generation != entity.generation()) return nullptr;
            return &found->second.value;
        }

        [[nodiscard]] Component* find(Entity entity) noexcept {
            const auto found = values_.find(entity.index());
            if (found == values_.end() ||
                found->second.generation != entity.generation()) return nullptr;
            return &found->second.value;
        }

        bool remove(Entity entity) noexcept {
            const auto found = values_.find(entity.index());
            if (found == values_.end() ||
                found->second.generation != entity.generation()) return false;
            values_.erase(found);
            return true;
        }

        void erase(Entity entity) noexcept override {
            (void)remove(entity);
        }

        [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

    private:
        struct Entry final {
            template <typename... Args>
            Entry(std::uint32_t entity_generation, Args&&... args)
                : generation(entity_generation),
                  value(std::forward<Args>(args)...) {}

            std::uint32_t generation;
            Component value;
        };

        std::unordered_map<std::uint32_t, Entry> values_;
    };

public:
    World() = default;
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    [[nodiscard]] Entity create_entity() {
        std::unique_lock lock(mutex_);
        const auto entity = handles_.allocate();
        if (!entity) return {};
        try {
            entities_.emplace(entity.index(), entity);
        } catch (...) {
            (void)handles_.release(entity);
            throw;
        }
        return entity;
    }

    [[nodiscard]] bool destroy_entity(Entity entity) {
        if (!entity) return false;
        std::unique_lock lock(mutex_);
        const auto found = entities_.find(entity.index());
        if (found == entities_.end() || found->second != entity) return false;

        // Release first because HandlePool may allocate while growing its
        // free-list. If that throws, the entity and its components stay intact.
        if (!handles_.release(entity)) return false;
        for (auto& [type, pool] : pools_) {
            (void)type;
            pool->erase(entity);
        }
        entities_.erase(found);
        return true;
    }

    [[nodiscard]] bool is_alive(Entity entity) const {
        if (!entity) return false;
        std::shared_lock lock(mutex_);
        const auto found = entities_.find(entity.index());
        return found != entities_.end() && found->second == entity &&
               handles_.is_alive(entity);
    }

    [[nodiscard]] std::size_t entity_count() const {
        std::shared_lock lock(mutex_);
        return entities_.size();
    }

    // Returns a stable, index-sorted snapshot. The returned entities may be
    // destroyed immediately after this method releases the world lock.
    [[nodiscard]] std::vector<Entity> entities() const {
        std::shared_lock lock(mutex_);
        std::vector<Entity> result;
        result.reserve(entities_.size());
        for (const auto& [index, entity] : entities_) {
            (void)index;
            result.push_back(entity);
        }
        return result;
    }

    template <typename Component, typename... Args>
    bool emplace_component(Entity entity, Args&&... args) {
        static_assert(!std::is_reference_v<Component> &&
                      !std::is_const_v<Component> &&
                      !std::is_volatile_v<Component>,
                      "Components must be unqualified object types");
        std::unique_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        return assure_pool_unlocked<Component>().emplace(
            entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    bool remove_component(Entity entity) {
        std::unique_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        auto* pool = find_pool_unlocked<Component>();
        return pool != nullptr && pool->remove(entity);
    }

    template <typename Component>
    [[nodiscard]] bool has_component(Entity entity) const {
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        const auto* pool = find_pool_unlocked<Component>();
        return pool != nullptr && pool->find(entity) != nullptr;
    }

    template <typename Component>
    [[nodiscard]] std::size_t component_count() const {
        std::shared_lock lock(mutex_);
        const auto* pool = find_pool_unlocked<Component>();
        return pool == nullptr ? 0 : pool->size();
    }

    template <typename Component, typename Function>
    bool with_component(Entity entity, Function&& function) const {
        static_assert(std::is_invocable_v<std::decay_t<Function>, const Component&> &&
                      std::is_same_v<
                          std::invoke_result_t<std::decay_t<Function>, const Component&>,
                          void>,
                      "Read callback must accept const Component& and return void");
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        const auto* pool = find_pool_unlocked<Component>();
        if (pool == nullptr) return false;
        const auto* component = pool->find(entity);
        if (component == nullptr) return false;
        std::invoke(std::forward<Function>(function), *component);
        return true;
    }

    template <typename Component, typename Function>
    bool with_component_mut(Entity entity, Function&& function) {
        static_assert(std::is_invocable_v<std::decay_t<Function>, Component&> &&
                      std::is_same_v<
                          std::invoke_result_t<std::decay_t<Function>, Component&>,
                          void>,
                      "Mutation callback must accept Component& and return void");
        std::unique_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        auto* pool = find_pool_unlocked<Component>();
        if (pool == nullptr) return false;
        auto* component = pool->find(entity);
        if (component == nullptr) return false;
        std::invoke(std::forward<Function>(function), *component);
        return true;
    }

    template <typename... Components, typename Function>
    bool with_components(Entity entity, Function&& function) const {
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, const Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, const Components&...>,
                          void>,
                      "Read callback must accept Entity and const component references");
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        const auto components = std::tuple<const Components*...>{
            find_component_unlocked<Components>(entity)...};
        if (!all_present(components)) return false;
        std::apply([&](auto... values) {
            std::invoke(std::forward<Function>(function), entity, *values...);
        }, components);
        return true;
    }

    template <typename... Components, typename Function>
    bool with_components_mut(Entity entity, Function&& function) {
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, Components&...>,
                          void>,
                      "Mutation callback must accept Entity and mutable component references");
        std::unique_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        const auto components = std::tuple<Components*...>{
            find_component_unlocked<Components>(entity)...};
        if (!all_present(components)) return false;
        std::apply([&](auto... values) {
            std::invoke(std::forward<Function>(function), entity, *values...);
        }, components);
        return true;
    }

    template <typename... Components, typename Function>
    std::size_t for_each(Function&& function) const {
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, const Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, const Components&...>,
                          void>,
                      "Iteration callback must accept Entity and const component references");
        std::shared_lock lock(mutex_);
        std::size_t visited = 0;
        for (const auto& [index, entity] : entities_) {
            (void)index;
            const auto components = std::tuple<const Components*...>{
                find_component_unlocked<Components>(entity)...};
            if (!all_present(components)) continue;
            std::apply([&](auto... values) {
                std::invoke(function, entity, *values...);
            }, components);
            ++visited;
        }
        return visited;
    }

    template <typename... Components, typename Function>
    std::size_t for_each_mut(Function&& function) {
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, Components&...>,
                          void>,
                      "Mutation iteration callback must accept Entity and mutable component references");
        std::unique_lock lock(mutex_);
        std::size_t visited = 0;
        for (const auto& [index, entity] : entities_) {
            (void)index;
            const auto components = std::tuple<Components*...>{
                find_component_unlocked<Components>(entity)...};
            if (!all_present(components)) continue;
            std::apply([&](auto... values) {
                std::invoke(function, entity, *values...);
            }, components);
            ++visited;
        }
        return visited;
    }

private:
    [[nodiscard]] bool is_alive_unlocked(Entity entity) const {
        if (!entity) return false;
        const auto found = entities_.find(entity.index());
        return found != entities_.end() && found->second == entity &&
               handles_.is_alive(entity);
    }

    template <typename Component>
    ComponentPool<Component>& assure_pool_unlocked() {
        const std::type_index type{typeid(Component)};
        const auto found = pools_.find(type);
        if (found != pools_.end())
            return *static_cast<ComponentPool<Component>*>(found->second.get());

        auto pool = std::make_unique<ComponentPool<Component>>();
        auto* result = pool.get();
        pools_.emplace(type, std::move(pool));
        return *result;
    }

    template <typename Component>
    [[nodiscard]] ComponentPool<Component>* find_pool_unlocked() {
        const auto found = pools_.find(std::type_index{typeid(Component)});
        return found == pools_.end()
            ? nullptr
            : static_cast<ComponentPool<Component>*>(found->second.get());
    }

    template <typename Component>
    [[nodiscard]] const ComponentPool<Component>* find_pool_unlocked() const {
        const auto found = pools_.find(std::type_index{typeid(Component)});
        return found == pools_.end()
            ? nullptr
            : static_cast<const ComponentPool<Component>*>(found->second.get());
    }

    template <typename Component>
    [[nodiscard]] const Component* find_component_unlocked(Entity entity) const {
        const auto* pool = find_pool_unlocked<Component>();
        return pool == nullptr ? nullptr : pool->find(entity);
    }

    template <typename Component>
    [[nodiscard]] Component* find_component_unlocked(Entity entity) {
        auto* pool = find_pool_unlocked<Component>();
        return pool == nullptr ? nullptr : pool->find(entity);
    }

    template <typename Tuple>
    static bool all_present(const Tuple& values) {
        return std::apply([](auto... pointers) {
            return (... && (pointers != nullptr));
        }, values);
    }

    mutable std::shared_mutex mutex_;
    core::HandlePool<EntityTag> handles_;
    std::map<std::uint32_t, Entity> entities_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
};

} // namespace dream::world
