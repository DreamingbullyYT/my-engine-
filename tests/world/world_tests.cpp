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

    const Transform2D parent2d{{10.0F, 20.0F}, 1.57079632679F, {2.0F, 2.0F}};
    const Transform2D child2d{{1.0F, 0.0F}, 0.0F, {1.0F, 1.0F}};
    const auto world2d = dream::world::multiply(
        dream::world::transform_matrix(parent2d),
        dream::world::transform_matrix(child2d));
    const auto point2d = dream::world::transform_point(world2d, {0.0F, 0.0F});
    require(point2d.x > 9.99F && point2d.x < 10.01F &&
            point2d.y > 21.99F && point2d.y < 22.01F,
            "2D transform composition applies parent scale, rotation and translation");

    const Transform3D parent3d{{3.0F, 4.0F, 5.0F}, {0, 0, 0, 1}, {2, 2, 2}};
    const Transform3D child3d{{1.0F, 0.0F, 0.0F}, {0, 0, 0, 1}, {1, 1, 1}};
    const auto world3d = dream::world::multiply(
        dream::world::transform_matrix(parent3d),
        dream::world::transform_matrix(child3d));
    const auto point3d = dream::world::transform_point(world3d, {0, 0, 0});
    require(point3d.x == 5.0F && point3d.y == 4.0F && point3d.z == 5.0F,
            "3D transform composition applies parent scale and translation");

    const float half_angle = 0.78539816339F;
    const Transform3D rotated3d{
        {}, {0.0F, 0.0F, std::sin(half_angle), std::cos(half_angle)}, {1, 1, 1}};
    const auto rotated_point = dream::world::transform_point(
        dream::world::transform_matrix(rotated3d), {1.0F, 0.0F, 0.0F});
    require(rotated_point.x > -0.001F && rotated_point.x < 0.001F &&
            rotated_point.y > 0.999F && rotated_point.y < 1.001F,
            "3D quaternion transform rotates vectors");

    World world;
    const auto invalid = dream::world::Entity{};
    require(!world.is_alive(invalid), "default entity is invalid");
    require(!world.destroy_entity(invalid), "invalid entity cannot be destroyed");

    const auto first = world.create_entity();
    const auto second = world.create_entity();
    const auto third = world.create_entity();
    require(first && second && third, "entities are created");
    require(world.entity_count() == 3, "entity count tracks creation");
    require(world.set_parent(second, first), "entity can be attached to a parent");
    require(world.set_parent(third, second), "nested entity parenting succeeds");
    require(world.parent_of(second) == first && world.parent_of(third) == second,
            "parent lookup returns exact generational entities");
    require(world.is_ancestor(first, third) && world.is_ancestor(second, third),
            "ancestor queries walk the complete hierarchy");
    require(!world.is_ancestor(third, first), "reverse ancestry is false");
    const auto first_children = world.children_of(first);
    require(first_children.size() == 1 && first_children[0] == second,
            "children query returns direct children in stable order");
    require(!world.set_parent(first, third), "parent assignment rejects cycles");
    require(!world.set_parent(first, first), "parent assignment rejects self-parenting");
    require(!world.set_parent(first, dream::world::Entity{}),
            "parent assignment rejects invalid handles");

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
            require(static_cast<bool>(entity), "multi-component callback receives entity");
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
    require(!world.parent_of(second), "destroying a parent detaches its direct children");
    require(!world.is_ancestor(first, third),
            "destroying a parent removes its hierarchy relationships");

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
    require(world.set_parent(third, second), "detached child can be attached again");
    require(world.clear_parent(third), "parent link can be cleared");
    require(!world.clear_parent(third), "clearing an absent parent link fails");

    std::cout << "Dream Engine world tests passed.\n";
    return EXIT_SUCCESS;
}
