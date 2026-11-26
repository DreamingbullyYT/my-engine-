#include <dream/world/world.hpp>
#include <dream/world/transform.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct Position { float x{0}; float y{0}; };
struct Velocity { float x{0}; float y{0}; };
struct Health { int value{100}; };
}

int main() {
    using dream::world::World;
    using dream::world::Transform2D;
    using dream::world::Transform3D;

    static_assert(sizeof(Transform2D) >= sizeof(float) * 5);
    static_assert(sizeof(Transform3D) >= sizeof(float) * 10);

    World world;
    const auto invalid = dream::world::Entity{};
    require(!world.is_alive(invalid), "default entity is invalid");
    require(!world.destroy_entity(invalid), "invalid entity cannot be destroyed");

    const auto first = world.create_entity();
    const auto second = world.create_entity();
    const auto third = world.create_entity();
    require(first && second && third, "entities are created");
    require(world.entity_count() == 3, "entity count tracks creation");

    require(world.emplace_component<Position>(first, 1.0F, 2.0F),
            "first position insertion succeeds");
    require(!world.emplace_component<Position>(first, 3.0F, 4.0F),
            "duplicate component is rejected");
    require(world.emplace_component<Velocity>(first, 0.5F, -1.0F),
            "velocity insertion succeeds");
    require(world.emplace_component<Health>(first, 80),
            "health insertion succeeds");
    require(world.emplace_component<Position>(second, 5.0F, 6.0F),
            "second entity position insertion succeeds");
    require(world.emplace_component<Health>(third, 40),
            "third entity health insertion succeeds");

    require(world.has_component<Position>(first), "component lookup finds position");
    require(!world.has_component<Velocity>(second), "missing component is reported");
    require(world.component_count<Position>() == 2, "component pool size is correct");

    bool read_ok = world.with_component<Position>(first, [](const Position& position) {
        require(position.x == 1.0F && position.y == 2.0F,
                "read callback observes inserted values");
    });
    require(read_ok, "read callback runs for existing component");

    bool write_ok = world.with_component_mut<Position>(first, [](Position& position) {
        position.x = 9.0F;
    });
    require(write_ok, "mutable callback runs");
    require(world.with_components<Position, Velocity>(
        first, [](dream::world::Entity entity, const Position& position,
                  const Velocity& velocity) {
            require(entity, "multi-component callback receives entity");
            require(position.x == 9.0F && velocity.x == 0.5F,
                    "multi-component callback receives matching components");
        }), "multi-component access succeeds");

    std::size_t visited = world.for_each<Position, Health>(
        [](dream::world::Entity, const Position&, const Health&) {});
    require(visited == 1, "multi-component iteration visits only matching entities");

    const auto changed = world.for_each_mut<Health>(
        [](dream::world::Entity, Health& health) { health.value += 1; });
    require(changed == 2, "mutable iteration visits all entities with Health");

    auto snapshot = world.entities();
    require(snapshot.size() == 3 && snapshot[0] == first &&
            snapshot[1] == second && snapshot[2] == third,
            "entity snapshots are sorted by handle index");

    require(world.remove_component<Velocity>(first), "component removal succeeds");
    require(!world.remove_component<Velocity>(first), "removing absent component fails");

    require(world.destroy_entity(first), "entity destruction succeeds");
    require(!world.is_alive(first), "destroyed entity becomes stale");
    require(!world.has_component<Position>(first), "destroyed entity loses components");
    require(!world.destroy_entity(first), "double entity destruction is rejected");
    require(world.entity_count() == 2, "entity count updates after destruction");

    const auto reused = world.create_entity();
    require(reused.index() == first.index() &&
            reused.generation() != first.generation(),
            "destroyed entity slot is reused with new generation");
    require(!world.has_component<Position>(reused),
            "reused entity does not inherit stale components");

    require(world.emplace_component<Transform2D>(reused),
            "2D transform can be stored as a component");
    require(world.emplace_component<Transform3D>(second),
            "3D transform can be stored as a component");

    std::cout << "Dream Engine world tests passed.\n";
    return EXIT_SUCCESS;
}
