#pragma once

#include <cstdint>

namespace dream::world {

struct Vec2 final {
    float x{0.0F};
    float y{0.0F};
};

struct Vec3 final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct Quaternion final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};
};

// Data-only transform components. 2D rotation is radians; 3D rotation is a
// quaternion. World/local-space composition is deliberately delegated to a
// future transform system so this header does not impose a math convention.
struct Transform2D final {
    Vec2 position{};
    float rotation_radians{0.0F};
    Vec2 scale{1.0F, 1.0F};
};

struct Transform3D final {
    Vec3 position{};
    Quaternion rotation{};
    Vec3 scale{1.0F, 1.0F, 1.0F};
};

struct Parent final {
    std::uint32_t entity_index{0xFFFFFFFFU};
    std::uint32_t entity_generation{0U};

    [[nodiscard]] constexpr bool has_parent() const noexcept {
        return entity_index != 0xFFFFFFFFU && entity_generation != 0U;
    }
};

} // namespace dream::world
