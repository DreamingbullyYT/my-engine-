#include <dream/physics2d/geometry.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace dream::physics2d;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool near(float left, float right, float epsilon = 0.001F) {
    return std::abs(left - right) <= epsilon;
}
}

int main() {
    const Aabb box_a{{0, 0}, {2, 2}};
    const Aabb box_b{{1, 0.5F}, {3, 1.5F}};
    const auto box_contact = collide(box_a, box_b);
    require(box_contact.has_value(), "overlapping AABBs collide");
    require(near(box_contact->normal.x, 1.0F) &&
            near(box_contact->normal.y, 0.0F),
            "AABB contact normal points from first to second");
    require(near(box_contact->penetration, 1.0F),
            "AABB penetration selects minimum overlap axis");
    require(intersects(box_a, box_b), "AABB broad overlap test succeeds");
    require(!intersects(box_a, Aabb{{2.01F, 0}, {3, 1}}),
            "separated AABBs do not intersect");
    require(collide(box_a, Aabb{{2, 0}, {3, 1}})->penetration == 0.0F,
            "touching AABBs produce a zero-penetration contact");

    const auto reverse_box_contact = collide(box_b, box_a);
    require(reverse_box_contact &&
            near(reverse_box_contact->normal.x, -box_contact->normal.x),
            "reversing AABB arguments reverses the normal");

    const Circle circle_a{{0, 0}, 1.0F};
    const Circle circle_b{{1.5F, 0}, 1.0F};
    const auto circle_contact = collide(circle_a, circle_b);
    require(circle_contact.has_value(), "overlapping circles collide");
    require(near(circle_contact->normal.x, 1.0F) &&
            near(circle_contact->normal.y, 0.0F),
            "circle contact normal is normalized");
    require(near(circle_contact->penetration, 0.5F),
            "circle penetration is radius sum minus distance");
    require(!collide(circle_a, Circle{{2.01F, 0}, 1.0F}),
            "separate circles do not collide");
    require(collide(circle_a, Circle{{0, 0}, 0.5F}).has_value(),
            "concentric circles use deterministic fallback normal");
    require(collide(Circle{{0, 0}, 0.0F}, Circle{{0, 0}, 0.0F}).has_value(),
            "coincident zero-radius circles touch");

    const Aabb unit_box{{-1, -1}, {1, 1}};
    const auto external_circle = collide(Circle{{1.5F, 0}, 0.75F}, unit_box);
    require(external_circle && near(external_circle->normal.x, -1.0F) &&
            near(external_circle->penetration, 0.25F),
            "circle-AABB collision reports exterior contact");
    const auto reverse_circle = collide(unit_box, Circle{{1.5F, 0}, 0.75F});
    require(reverse_circle && near(reverse_circle->normal.x, 1.0F),
            "AABB-circle collision reverses the normal");

    const auto inside_circle = collide(Circle{{0.8F, 0}, 0.5F}, unit_box);
    require(inside_circle && near(inside_circle->normal.x, -1.0F) &&
            near(inside_circle->penetration, 0.7F),
            "circle centre inside AABB selects nearest exit face");

    const OrientedBox rotated_box{{0, 0}, {2, 0.5F}, 0.78539816339F};
    const auto obb_bounds = rotated_box.bounds();
    require(obb_bounds.valid() && obb_bounds.maximum.x > 1.7F,
            "oriented box computes conservative AABB bounds");
    const auto circle_obb = collide(
        Circle{{1.2F, 0.0F}, 0.4F}, rotated_box);
    require(circle_obb.has_value(),
            "circle collides with rotated oriented box");
    const auto box_circle = collide(
        rotated_box, Circle{{1.2F, 0.0F}, 0.4F});
    require(box_circle &&
            near(box_circle->normal.x, -circle_obb->normal.x) &&
            near(box_circle->normal.y, -circle_obb->normal.y),
            "OBB-circle overload reverses normal");

    const OrientedBox obb_a{{0, 0}, {1, 1}, 0.0F};
    const OrientedBox obb_b{{1.5F, 0}, {1, 1}, 0.0F};
    const auto obb_contact = collide(obb_a, obb_b);
    require(obb_contact && near(obb_contact->penetration, 0.5F) &&
            near(obb_contact->normal.x, 1.0F),
            "oriented-box SAT detects overlapping boxes");
    require(!collide(obb_a, OrientedBox{{3, 0}, {1, 1}, 0.4F}),
            "oriented-box SAT rejects separated boxes");

    require(!Aabb{{2, 0}, {1, 1}}.valid(), "inverted AABB is invalid");
    require(!Circle{{0, 0}, -1.0F}.valid(), "negative circle radius is invalid");
    require(!Circle{{0, 0}, std::numeric_limits<float>::infinity()}.valid(),
            "infinite circle radius is invalid");
    require(!OrientedBox{{0, 0}, {-1, 1}, 0}.valid(),
            "negative OBB half extents are invalid");
    require(!OrientedBox{{0, 0}, {1, 1},
                std::numeric_limits<float>::quiet_NaN()}.valid(),
            "non-finite OBB rotation is invalid");

    std::cout << "Dream Engine 2D collision geometry tests passed.\n";
    return EXIT_SUCCESS;
}
