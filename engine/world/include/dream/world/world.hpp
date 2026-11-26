#pragma once

#include <dream/core/handle.hpp>

#include <algorithm>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dream::world {

struct EntityTag;
using Entity = core::Handle<EntityTag>;
class World;
enum class WorldMutationKind : std::uint8_t { EntityCreate, EntityDestroy, ComponentEmplace, ComponentRemove, ComponentModify, ComponentsModify, ComponentsIterateMut, ParentSet, ParentClear };
using WorldMutationOriginalV1 = void (*)(void*) noexcept;
struct WorldMutationContext final {
    World* world{};
    WorldMutationKind kind{WorldMutationKind::EntityCreate};
    Entity entity{};
    Entity related_entity{};
    std::type_index component_type{typeid(void)};
    bool succeeded{false};
    std::size_t affected_entities{0};
    // Private bridge payload used by EngineRuntime's hook trampoline.
    WorldMutationOriginalV1 original_operation{};
    void* original_context{};
};
using WorldMutationDispatcherV1 = void (*)(void*, std::string_view, WorldMutationContext*, WorldMutationOriginalV1, void*) noexcept;

// A thread-safe entity/component store. Structural changes are serialized.
// Component callbacks execute while the world's lock is held and must not
// re-enter this World. Read callbacks receive const components; mutation
// callbacks take the exclusive lock and receive mutable references.
class World final {
private:
    struct IComponentPool {
        virtual ~IComponentPool() = default;
        [[nodiscard]] virtual bool contains(Entity entity) const noexcept = 0;
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

        [[nodiscard]] bool contains(Entity entity) const noexcept override {
            return find(entity) != nullptr;
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
    void set_mutation_dispatcher(void* context, WorldMutationDispatcherV1 dispatcher) noexcept {
        std::lock_guard lock(mutation_dispatcher_mutex_);
        mutation_dispatch_context_ = context; mutation_dispatcher_ = dispatcher;
    }
    World& operator=(const World&) = delete;

    [[nodiscard]] Entity create_entity() {
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::EntityCreate;
        run_mutation("dream.engine.world.entity.create", mutation, [&] {
            std::unique_lock lock(mutex_); const auto entity = handles_.allocate(); if (!entity) return;
            try { entities_.emplace(entity.index(), entity); }
            catch (...) { (void)handles_.release(entity); throw; }
            mutation.entity = entity; mutation.succeeded = true;
        });
        return mutation.succeeded ? mutation.entity : Entity{};
    }

    [[nodiscard]] bool destroy_entity(Entity entity) {
        if (!entity) return false;
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::EntityDestroy; mutation.entity = entity;
        run_mutation("dream.engine.world.entity.destroy", mutation, [&] {
            std::unique_lock lock(mutex_); const auto found = entities_.find(entity.index());
            if (found == entities_.end() || found->second != entity) return;
            if (!handles_.release(entity)) return;
            for (auto& [type, pool] : pools_) { (void)type; pool->erase(entity); }
            parents_.erase(entity.index());
            for (auto parent = parents_.begin(); parent != parents_.end();) { if (parent->second == entity) parent = parents_.erase(parent); else ++parent; }
            entities_.erase(found); mutation.succeeded = true;
        });
        return mutation.succeeded;
    }

    [[nodiscard]] bool is_alive(Entity entity) const {
        if (!entity) return false;
        std::shared_lock lock(mutex_);
        const auto found = entities_.find(entity.index());
        return found != entities_.end() && found->second == entity &&
               handles_.is_alive(entity);
    }

    // Parent links are stored by the World rather than as an ordinary
    // component, so cycle checks, reparenting and entity destruction are atomic
    // with respect to other world mutations.
    [[nodiscard]] bool set_parent(Entity child, Entity parent) {
        if (!child || !parent || child == parent) return false;
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ParentSet; mutation.entity = child; mutation.related_entity = parent;
        run_mutation("dream.engine.world.parent.set", mutation, [&] {
            std::unique_lock lock(mutex_); if (!is_alive_unlocked(child) || !is_alive_unlocked(parent)) return;
            Entity current = parent; std::size_t traversed = 0;
            while (current) { if (current == child || ++traversed > parents_.size() + 1U) return; if (!is_alive_unlocked(current)) return; const auto found = parents_.find(current.index()); if (found == parents_.end()) break; current = found->second; }
            parents_.insert_or_assign(child.index(), parent); mutation.succeeded = true;
        });
        return mutation.succeeded;
    }
    [[nodiscard]] bool clear_parent(Entity child) {
        if (!child) return false;
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ParentClear; mutation.entity = child;
        run_mutation("dream.engine.world.parent.clear", mutation, [&] { std::unique_lock lock(mutex_); if (is_alive_unlocked(child)) mutation.succeeded = parents_.erase(child.index()) != 0; });
        return mutation.succeeded;
    }

    [[nodiscard]] Entity parent_of(Entity child) const {
        if (!child) return {};
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(child)) return {};
        const auto found = parents_.find(child.index());
        if (found == parents_.end() || !is_alive_unlocked(found->second)) return {};
        return found->second;
    }

    [[nodiscard]] std::vector<Entity> children_of(Entity parent) const {
        std::vector<Entity> result;
        if (!parent) return result;
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(parent)) return result;
        for (const auto& [child_index, candidate_parent] : parents_) {
            if (candidate_parent != parent) continue;
            const auto child = entities_.find(child_index);
            if (child != entities_.end() && is_alive_unlocked(child->second))
                result.push_back(child->second);
        }
        std::sort(result.begin(), result.end(),
            [](Entity left, Entity right) { return left.index() < right.index(); });
        return result;
    }

    // Strict ancestry: an entity is not its own ancestor.
    [[nodiscard]] bool is_ancestor(Entity ancestor, Entity descendant) const {
        if (!ancestor || !descendant || ancestor == descendant) return false;
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(ancestor) || !is_alive_unlocked(descendant)) return false;
        Entity current = descendant;
        std::size_t traversed = 0;
        while (current) {
            if (++traversed > parents_.size() + 1U) return false;
            const auto found = parents_.find(current.index());
            if (found == parents_.end()) return false;
            current = found->second;
            if (current == ancestor) return true;
            if (!is_alive_unlocked(current)) return false;
        }
        return false;
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
        static_assert(valid_component_type<Component>, "Components must be unqualified object types");
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ComponentEmplace; mutation.entity = entity; mutation.component_type = std::type_index{typeid(Component)};
        run_mutation("dream.engine.world.component.emplace", mutation, [&] { std::unique_lock lock(mutex_); if (is_alive_unlocked(entity)) mutation.succeeded = assure_pool_unlocked<Component>().emplace(entity, std::forward<Args>(args)...); });
        return mutation.succeeded;
    }

    template <typename Component>
    bool remove_component(Entity entity) {
        static_assert(valid_component_type<Component>, "Component type must be an unqualified object type");
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ComponentRemove; mutation.entity = entity; mutation.component_type = std::type_index{typeid(Component)};
        run_mutation("dream.engine.world.component.remove", mutation, [&] { std::unique_lock lock(mutex_); if (!is_alive_unlocked(entity)) return; auto* pool = find_pool_unlocked<Component>(); mutation.succeeded = pool != nullptr && pool->remove(entity); });
        return mutation.succeeded;
    }

    template <typename Component>
    [[nodiscard]] bool has_component(Entity entity) const {
        static_assert(valid_component_type<Component>,
                      "Component type must be an unqualified object type");
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return false;
        const auto* pool = find_pool_unlocked<Component>();
        return pool != nullptr && pool->find(entity) != nullptr;
    }

    template <typename Component>
    [[nodiscard]] std::size_t component_count() const {
        static_assert(valid_component_type<Component>,
                      "Component type must be an unqualified object type");
        std::shared_lock lock(mutex_);
        const auto* pool = find_pool_unlocked<Component>();
        return pool == nullptr ? 0 : pool->size();
    }

    // Runtime type tokens are for in-process reflection/registration only.
    // Persistent formats must use explicit stable IDs, never typeid names.
    [[nodiscard]] std::vector<std::type_index> component_types(Entity entity) const {
        std::vector<std::type_index> result;
        if (!entity) return result;
        std::shared_lock lock(mutex_);
        if (!is_alive_unlocked(entity)) return result;
        result.reserve(pools_.size());
        for (const auto& [type, pool] : pools_) {
            if (pool->contains(entity)) result.push_back(type);
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    template <typename Component, typename Function>
    bool with_component(Entity entity, Function&& function) const {
        static_assert(valid_component_type<Component>,
                      "Component type must be an unqualified object type");
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
        static_assert(valid_component_type<Component>,
                      "Component type must be an unqualified object type");
        static_assert(std::is_invocable_v<std::decay_t<Function>, Component&> &&
                      std::is_same_v<
                          std::invoke_result_t<std::decay_t<Function>, Component&>,
                          void>,
                      "Mutation callback must accept Component& and return void");
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ComponentModify; mutation.entity = entity; mutation.component_type = std::type_index{typeid(Component)};
        run_mutation("dream.engine.world.component.modify", mutation, [&] {
            std::unique_lock lock(mutex_); if (!is_alive_unlocked(entity)) return;
            auto* pool = find_pool_unlocked<Component>(); if (!pool) return;
            auto* component = pool->find(entity); if (!component) return;
            std::invoke(std::forward<Function>(function), *component);
            mutation.succeeded = true; mutation.affected_entities = 1U;
        });
        return mutation.succeeded;
    }

    template <typename... Components, typename Function>
    bool with_components(Entity entity, Function&& function) const {
        static_assert((valid_component_type<Components> && ...),
                      "Component types must be unqualified object types");
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
        static_assert((valid_component_type<Components> && ...),
                      "Component types must be unqualified object types");
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, Components&...>,
                          void>,
                      "Mutation callback must accept Entity and mutable component references");
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ComponentsModify; mutation.entity = entity; mutation.component_type = std::type_index{typeid(std::tuple<Components...>)};
        run_mutation("dream.engine.world.components.modify", mutation, [&] {
            std::unique_lock lock(mutex_); if (!is_alive_unlocked(entity)) return;
            const auto components = std::tuple<Components*...>{find_component_unlocked<Components>(entity)...};
            if (!all_present(components)) return;
            std::apply([&](auto... values) { std::invoke(std::forward<Function>(function), entity, *values...); }, components);
            mutation.succeeded = true; mutation.affected_entities = 1U;
        });
        return mutation.succeeded;
    }

    template <typename... Components, typename Function>
    std::size_t for_each(Function&& function) const {
        static_assert((valid_component_type<Components> && ...),
                      "Component types must be unqualified object types");
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
        static_assert((valid_component_type<Components> && ...),
                      "Component types must be unqualified object types");
        static_assert(sizeof...(Components) > 0, "At least one component type is required");
        static_assert(std::is_invocable_v<
                          std::decay_t<Function>, Entity, Components&...> &&
                      std::is_same_v<
                          std::invoke_result_t<
                              std::decay_t<Function>, Entity, Components&...>,
                          void>,
                      "Mutation iteration callback must accept Entity and mutable component references");
        WorldMutationContext mutation; mutation.world = this; mutation.kind = WorldMutationKind::ComponentsIterateMut; mutation.component_type = std::type_index{typeid(std::tuple<Components...>)};
        run_mutation("dream.engine.world.components.iterate_mut", mutation, [&] {
            std::unique_lock lock(mutex_); std::size_t visited = 0U;
            for (const auto& [index, entity] : entities_) {
                (void)index; const auto components = std::tuple<Components*...>{find_component_unlocked<Components>(entity)...};
                if (!all_present(components)) continue;
                std::apply([&](auto... values) { std::invoke(function, entity, *values...); }, components);
                ++visited; mutation.affected_entities = visited;
            }
            mutation.succeeded = true;
        });
        return mutation.affected_entities;
    }

private:
    template <typename Function>
    void run_mutation(std::string_view hook_point, WorldMutationContext& mutation, Function&& original) {
        using Callable = std::decay_t<Function>; Callable callable(std::forward<Function>(original));
        struct Operation final { Callable* callable{}; std::exception_ptr failure{}; bool invoked{false}; };
        Operation operation{&callable, {}, false};
        const auto invoke_original = [](void* opaque) noexcept {
            auto& call = *static_cast<Operation*>(opaque); if (call.invoked) return; call.invoked = true;
            try { std::invoke(*call.callable); } catch (...) { call.failure = std::current_exception(); }
        };
        WorldMutationDispatcherV1 dispatcher = nullptr; void* context = nullptr;
        { std::lock_guard lock(mutation_dispatcher_mutex_); dispatcher = mutation_dispatcher_; context = mutation_dispatch_context_; }
        if (dispatcher) dispatcher(context, hook_point, &mutation, invoke_original, &operation);
        else invoke_original(&operation);
        if (operation.failure) std::rethrow_exception(operation.failure);
    }

    template <typename Component>
    static constexpr bool valid_component_type =
        std::is_object_v<Component> &&
        std::is_same_v<Component, std::remove_cv_t<Component>>;

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
    std::unordered_map<std::uint32_t, Entity> parents_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
    mutable std::mutex mutation_dispatcher_mutex_;
    void* mutation_dispatch_context_{nullptr};
    WorldMutationDispatcherV1 mutation_dispatcher_{nullptr};
};

} // namespace dream::world
