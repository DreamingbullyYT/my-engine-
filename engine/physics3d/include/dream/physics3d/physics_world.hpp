#pragma once

#include <dream/core/handle.hpp>
#include <dream/physics3d/broadphase.hpp>
#include <dream/physics3d/geometry.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace dream::physics3d {

struct BodyTag3D;
using BodyHandle3D = core::Handle<BodyTag3D>;

struct Quaternion3D final {
    float x{0.0F}, y{0.0F}, z{0.0F}, w{1.0F};
};

[[nodiscard]] inline Quaternion3D normalized(Quaternion3D q) noexcept {
    const float n2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!std::isfinite(n2)||n2<1.0e-12F) return {};
    const float inverse=1.0F/std::sqrt(n2);
    return {q.x*inverse,q.y*inverse,q.z*inverse,q.w*inverse};
}
[[nodiscard]] inline Quaternion3D multiply(
    Quaternion3D a,Quaternion3D b) noexcept {
    return {
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z
    };
}
[[nodiscard]] inline Vec3 rotate(Vec3 v,Quaternion3D q) noexcept {
    q=normalized(q);
    const Vec3 u{q.x,q.y,q.z};
    return v+cross(u,cross(u,v)+v*q.w)*2.0F;
}
[[nodiscard]] inline Vec3 inverse_rotate(Vec3 v,Quaternion3D q) noexcept {
    return rotate(v,Quaternion3D{-q.x,-q.y,-q.z,q.w});
}

enum class BodyType3D : std::uint8_t { Static, Kinematic, Dynamic };

struct SphereCollider3D final {
    Vec3 local_center{};
    float radius{0.5F};
};
struct BoxCollider3D final {
    Vec3 local_center{};
    Vec3 half_extents{0.5F,0.5F,0.5F};
    Quaternion3D local_rotation{};
};
using ColliderShape3D = std::variant<SphereCollider3D,BoxCollider3D>;

struct CollisionFilter3D final {
    std::uint32_t category_bits{0x00000001U};
    std::uint32_t mask_bits{0xFFFFFFFFU};
    std::int16_t group_index{0};
};

struct BodyDescription3D final {
    BodyType3D type{BodyType3D::Static};
    Vec3 position{};
    Quaternion3D rotation{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    float mass{1.0F};
    float linear_damping{0.0F};
    float angular_damping{0.0F};
    float friction{0.5F};
    float restitution{0.0F};
    ColliderShape3D collider{SphereCollider3D{}};
    CollisionFilter3D collision_filter{};
    bool sensor{false};
};

struct PhysicsSettings3D final {
    Vec3 gravity{0.0F,-9.81F,0.0F};
    std::uint32_t velocity_iterations{8U};
    std::uint32_t position_iterations{3U};
    float penetration_slop{0.005F};
    float position_correction_percent{0.8F};
    float restitution_velocity_threshold{1.0F};
    float maximum_delta_seconds{0.25F};
};

enum class BodyCreateError3D : std::uint8_t {
    None, InvalidBodyType, InvalidTransform, InvalidVelocity,
    InvalidMass, InvalidMaterial, InvalidDamping, InvalidCollider,
    InvalidInertia, InvalidWorldBounds, CapacityExhausted
};
enum class PhysicsStepError3D : std::uint8_t {
    None, InvalidDelta, DeltaTooLarge, InvalidSettings, InvalidBodyState
};
enum class ContactEventType3D : std::uint8_t { Begin, Stay, End };

struct BodyCreateResult3D final {
    BodyHandle3D handle{};
    BodyCreateError3D error{BodyCreateError3D::None};
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle)&&error==BodyCreateError3D::None;
    }
};
struct BodyState3D final {
    BodyHandle3D handle{};
    BodyType3D type{BodyType3D::Static};
    Vec3 position{};
    Quaternion3D rotation{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    float mass{0.0F};
    bool sensor{false};
    CollisionFilter3D collision_filter{};
};
struct PhysicsContact3D final {
    BodyHandle3D first{};
    BodyHandle3D second{};
    ContactManifold3D manifold{};
    float normal_impulse{0.0F};
    float tangent_impulse{0.0F};
    bool sensor{false};
};
struct ContactEvent3D final {
    ContactEventType3D type{ContactEventType3D::Begin};
    PhysicsContact3D contact{};
};
struct PhysicsStepReport3D final {
    PhysicsStepError3D error{PhysicsStepError3D::None};
    std::size_t body_count{0};
    std::size_t broadphase_pair_count{0};
    std::size_t contact_count{0};
    std::vector<PhysicsContact3D> contacts;
    std::vector<ContactEvent3D> events;
    [[nodiscard]] explicit operator bool() const noexcept {
        return error==PhysicsStepError3D::None;
    }
};

// A fixed-step, single-thread-owned 3D rigid-body world. It supports sphere and
// oriented-box colliders, a deterministic 3D sweep-and-prune broadphase,
// sequential normal/friction impulses and bounded positional correction.
// One collider is attached to each body. Joints, CCD and sleeping are separate
// extensions; the OBB contact point is a support-point approximation.
class PhysicsWorld3D final {
public:
    explicit PhysicsWorld3D(PhysicsSettings3D settings={}) noexcept
        : settings_(settings),settings_valid_(valid_settings(settings)) {}
    PhysicsWorld3D(const PhysicsWorld3D&)=delete;
    PhysicsWorld3D& operator=(const PhysicsWorld3D&)=delete;

    [[nodiscard]] bool valid() const noexcept { return settings_valid_; }
    [[nodiscard]] const PhysicsSettings3D& settings() const noexcept { return settings_; }
    [[nodiscard]] bool configure(PhysicsSettings3D settings) noexcept {
        if(!valid_settings(settings)) return false;
        settings_=settings;settings_valid_=true;return true;
    }
    [[nodiscard]] std::size_t body_count() const noexcept { return bodies_.size(); }
    [[nodiscard]] const Broadphase3D& broadphase() const noexcept { return broadphase_; }

    [[nodiscard]] BodyCreateResult3D create_body(BodyDescription3D description) {
        const auto validation=validate_description(description);
        if(validation!=BodyCreateError3D::None) return {{},validation};
        const auto handle=handles_.allocate();
        if(!handle) return {{},BodyCreateError3D::CapacityExhausted};
        Body body=make_body(handle,std::move(description));
        if(body.type==BodyType3D::Dynamic&&
            (!finite(body.inverse_inertia_local)||
             body.inverse_inertia_local.x<=0.0F||
             body.inverse_inertia_local.y<=0.0F||
             body.inverse_inertia_local.z<=0.0F)) {
            (void)handles_.release(handle);
            return {{},BodyCreateError3D::InvalidInertia};
        }
        try {
            auto [it,inserted]=bodies_.emplace(handle.index(),std::move(body));
            if(!inserted||!update_proxy(it->second)) {
                if(inserted) bodies_.erase(it);
                (void)handles_.release(handle);
                return {{},BodyCreateError3D::InvalidWorldBounds};
            }
        } catch(...) {
            (void)handles_.release(handle);
            throw;
        }
        return {handle,BodyCreateError3D::None};
    }

    [[nodiscard]] bool destroy_body(BodyHandle3D handle) {
        auto* body=find_body(handle);
        if(body==nullptr||!handles_.release(handle)) return false;
        bodies_.erase(handle.index());
        (void)broadphase_.remove(proxy_id(handle));
        return true;
    }
    [[nodiscard]] bool is_alive(BodyHandle3D handle) const noexcept {
        return find_body(handle)!=nullptr;
    }
    [[nodiscard]] std::optional<BodyState3D> body_state(BodyHandle3D handle) const {
        const auto* body=find_body(handle);
        if(!body) return std::nullopt;
        return BodyState3D{body->handle,body->type,body->position,body->rotation,
            body->linear_velocity,body->angular_velocity,body->mass,
            body->sensor,body->collision_filter};
    }
    [[nodiscard]] bool set_velocity(
        BodyHandle3D handle,Vec3 linear,Vec3 angular={}) noexcept {
        auto* body=find_body(handle);
        if(!body||body->type==BodyType3D::Static||!finite(linear)||!finite(angular))
            return false;
        body->linear_velocity=linear;body->angular_velocity=angular;return true;
    }
    [[nodiscard]] bool apply_force(BodyHandle3D handle,Vec3 force,Vec3 point={}) noexcept {
        auto* body=find_body(handle);
        if(!body||body->type!=BodyType3D::Dynamic||!finite(force)||!finite(point))
            return false;
        const Vec3 next_force=body->force+force;
        const Vec3 next_torque=body->torque+cross(point-body->position,force);
        if(!finite(next_force)||!finite(next_torque)) return false;
        body->force=next_force;
        body->torque=next_torque;
        return true;
    }
    [[nodiscard]] bool apply_impulse(BodyHandle3D handle,Vec3 impulse,Vec3 point) noexcept {
        auto* body=find_body(handle);
        if(!body||body->type!=BodyType3D::Dynamic||!finite(impulse)||!finite(point))
            return false;
        const Vec3 old_linear=body->linear_velocity;
        const Vec3 old_angular=body->angular_velocity;
        apply_impulse_to(*body,impulse,point-body->position);
        if(!finite(body->linear_velocity)||!finite(body->angular_velocity)) {
            body->linear_velocity=old_linear;
            body->angular_velocity=old_angular;
            return false;
        }
        return true;
    }
    [[nodiscard]] bool set_gravity(Vec3 gravity) noexcept {
        if(!finite(gravity)) return false;
        settings_.gravity=gravity;return true;
    }

    [[nodiscard]] PhysicsStepReport3D update(double delta_seconds) {
        PhysicsStepReport3D report;
        report.body_count=bodies_.size();
        if(!settings_valid_) { report.error=PhysicsStepError3D::InvalidSettings;return report; }
        if(!std::isfinite(delta_seconds)||delta_seconds<0.0) {
            report.error=PhysicsStepError3D::InvalidDelta;return report;
        }
        if(delta_seconds>settings_.maximum_delta_seconds) {
            report.error=PhysicsStepError3D::DeltaTooLarge;return report;
        }
        if(delta_seconds==0.0) return report;
        const float dt=static_cast<float>(delta_seconds);
        const auto rollback=bodies_;
        try {
            for(auto& [index,body]:bodies_) {
                (void)index;
                if(body.type==BodyType3D::Dynamic) {
                    body.linear_velocity=(body.linear_velocity+
                        (settings_.gravity+body.force*body.inverse_mass)*dt)*
                        std::max(0.0F,1.0F-body.linear_damping*dt);
                    body.angular_velocity=(body.angular_velocity+
                        inverse_inertia_world(body,body.torque)*dt)*
                        std::max(0.0F,1.0F-body.angular_damping*dt);
                }
                if(body.type!=BodyType3D::Static) {
                    body.position=body.position+body.linear_velocity*dt;
                    body.rotation=integrate_rotation(body.rotation,body.angular_velocity,dt);
                }
                body.force={};body.torque={};
                if(!finite(body.position)||!finite(body.rotation)||
                   !finite(body.linear_velocity)||!finite(body.angular_velocity)||
                   !update_proxy(body)) {
                    bodies_=rollback;
                    rebuild_broadphase();
                    report.error=PhysicsStepError3D::InvalidBodyState;
                    return report;
                }
            }

            const auto candidate_pairs=broadphase_.compute_pairs();
            report.broadphase_pair_count=candidate_pairs.size();
            std::map<PairKey,PhysicsContact3D> current_contacts;
            for(const auto& pair:candidate_pairs) {
                const auto first_it=bodies_.find(static_cast<std::uint32_t>(pair.first-1U));
                const auto second_it=bodies_.find(static_cast<std::uint32_t>(pair.second-1U));
                if(first_it==bodies_.end()||second_it==bodies_.end()) continue;
                auto& first=first_it->second;
                auto& second=second_it->second;
                if(!should_collide(first.collision_filter,second.collision_filter)) continue;
                const auto manifold=collide_shapes(world_shape(first),world_shape(second));
                if(!manifold) continue;
                const PairKey key{pair.first,pair.second};
                current_contacts.emplace(key,PhysicsContact3D{
                    first.handle,second.handle,*manifold,0.0F,0.0F,
                    first.sensor||second.sensor});
            }

            solve_contacts(current_contacts,dt);
            report.contacts.reserve(current_contacts.size());
            for(auto& [key,contact]:current_contacts) {
                const auto previous=active_contacts_.find(key);
                report.events.push_back({previous==active_contacts_.end()
                    ?ContactEventType3D::Begin:ContactEventType3D::Stay,contact});
                report.contacts.push_back(contact);
            }
            for(const auto& [key,contact]:active_contacts_) {
                if(!current_contacts.contains(key))
                    report.events.push_back({ContactEventType3D::End,contact});
            }
            active_contacts_=std::move(current_contacts);
            for(auto& [index,body]:bodies_) {
                (void)index;
                if(!update_proxy(body)) {
                    bodies_=rollback;
                    rebuild_broadphase();
                    report.error=PhysicsStepError3D::InvalidBodyState;
                    return report;
                }
            }
            report.contact_count=report.contacts.size();
            report.body_count=bodies_.size();
            return report;
        } catch(...) {
            bodies_=rollback;
            rebuild_broadphase();
            throw;
        }
    }

private:
    struct Body final {
        BodyHandle3D handle{};
        BodyType3D type{BodyType3D::Static};
        Vec3 position{};
        Quaternion3D rotation{};
        Vec3 linear_velocity{};
        Vec3 angular_velocity{};
        float mass{0.0F},inverse_mass{0.0F};
        Vec3 inverse_inertia_local{};
        float linear_damping{0.0F},angular_damping{0.0F};
        float friction{0.5F},restitution{0.0F};
        ColliderShape3D collider{};
        CollisionFilter3D collision_filter{};
        bool sensor{false};
        Vec3 force{},torque{};
    };
    using PairKey=std::pair<BroadphaseProxyId3D,BroadphaseProxyId3D>;
    using WorldShape=std::variant<Sphere3D,OrientedBox3D>;

    [[nodiscard]] static BroadphaseProxyId3D proxy_id(BodyHandle3D h) noexcept {
        return static_cast<BroadphaseProxyId3D>(h.index())+1U;
    }
    [[nodiscard]] Body* find_body(BodyHandle3D h) noexcept {
        if(!h) return nullptr;
        const auto it=bodies_.find(h.index());
        return it!=bodies_.end()&&it->second.handle==h?&it->second:nullptr;
    }
    [[nodiscard]] const Body* find_body(BodyHandle3D h) const noexcept {
        if(!h) return nullptr;
        const auto it=bodies_.find(h.index());
        return it!=bodies_.end()&&it->second.handle==h?&it->second:nullptr;
    }
    [[nodiscard]] static bool finite(Quaternion3D q) noexcept {
        return std::isfinite(q.x)&&std::isfinite(q.y)&&
            std::isfinite(q.z)&&std::isfinite(q.w);
    }
    [[nodiscard]] static bool finite(Vec3 value) noexcept {
        return physics3d::finite(value);
    }
    [[nodiscard]] static bool valid_quaternion(Quaternion3D q) noexcept {
        if(!finite(q)) return false;
        const float n2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        return std::isfinite(n2)&&n2>1.0e-12F;
    }
    [[nodiscard]] static bool valid_settings(const PhysicsSettings3D& s) noexcept {
        return finite(s.gravity)&&s.velocity_iterations>0U&&
            s.velocity_iterations<=128U&&s.position_iterations>0U&&
            s.position_iterations<=32U&&std::isfinite(s.penetration_slop)&&
            s.penetration_slop>=0.0F&&
            std::isfinite(s.position_correction_percent)&&
            s.position_correction_percent>=0.0F&&
            s.position_correction_percent<=1.0F&&
            std::isfinite(s.restitution_velocity_threshold)&&
            s.restitution_velocity_threshold>=0.0F&&
            std::isfinite(s.maximum_delta_seconds)&&s.maximum_delta_seconds>0.0F;
    }
    [[nodiscard]] static BodyCreateError3D validate_description(
        const BodyDescription3D& d) noexcept {
        switch(d.type) {
        case BodyType3D::Static: case BodyType3D::Kinematic: case BodyType3D::Dynamic: break;
        default: return BodyCreateError3D::InvalidBodyType;
        }
        if(!finite(d.position)||!valid_quaternion(d.rotation))
            return BodyCreateError3D::InvalidTransform;
        if(!finite(d.linear_velocity)||!finite(d.angular_velocity))
            return BodyCreateError3D::InvalidVelocity;
        if(d.type==BodyType3D::Dynamic&&
           (!std::isfinite(d.mass)||d.mass<=1.0e-8F))
            return BodyCreateError3D::InvalidMass;
        if(!std::isfinite(d.friction)||d.friction<0.0F||
           !std::isfinite(d.restitution)||d.restitution<0.0F||d.restitution>1.0F)
            return BodyCreateError3D::InvalidMaterial;
        if(!std::isfinite(d.linear_damping)||d.linear_damping<0.0F||
           !std::isfinite(d.angular_damping)||d.angular_damping<0.0F)
            return BodyCreateError3D::InvalidDamping;
        if(const auto* sphere=std::get_if<SphereCollider3D>(&d.collider)) {
            if(!finite(sphere->local_center)||!std::isfinite(sphere->radius)||
               sphere->radius<=1.0e-6F)
                return BodyCreateError3D::InvalidCollider;
        } else {
            const auto& box=std::get<BoxCollider3D>(d.collider);
            if(!finite(box.local_center)||!finite(box.half_extents)||
               box.half_extents.x<=1.0e-6F||box.half_extents.y<=1.0e-6F||
               box.half_extents.z<=1.0e-6F||!valid_quaternion(box.local_rotation))
                return BodyCreateError3D::InvalidCollider;
        }
        return BodyCreateError3D::None;
    }
    [[nodiscard]] static Body make_body(
        BodyHandle3D handle,BodyDescription3D d) noexcept {
        Body b;
        b.handle=handle;b.type=d.type;b.position=d.position;
        b.rotation=normalized(d.rotation);b.linear_velocity=d.linear_velocity;
        b.angular_velocity=d.angular_velocity;b.mass=d.type==BodyType3D::Dynamic?d.mass:0.0F;
        b.inverse_mass=b.mass>0.0F?1.0F/b.mass:0.0F;
        b.linear_damping=d.linear_damping;b.angular_damping=d.angular_damping;
        b.friction=d.friction;b.restitution=d.restitution;
        b.collider=std::move(d.collider);b.collision_filter=d.collision_filter;
        b.sensor=d.sensor;
        if(b.type==BodyType3D::Dynamic) {
            Vec3 inertia{};
            if(const auto* sphere=std::get_if<SphereCollider3D>(&b.collider)) {
                const float base=0.4F*b.mass*sphere->radius*sphere->radius;
                const auto c=sphere->local_center;
                inertia={base+b.mass*(c.y*c.y+c.z*c.z),
                    base+b.mass*(c.x*c.x+c.z*c.z),
                    base+b.mass*(c.x*c.x+c.y*c.y)};
            } else {
                const auto& box=std::get<BoxCollider3D>(b.collider);
                const auto h=box.half_extents;
                inertia={
                    b.mass*(h.y*h.y+h.z*h.z)/3.0F,
                    b.mass*(h.x*h.x+h.z*h.z)/3.0F,
                    b.mass*(h.x*h.x+h.y*h.y)/3.0F};
                const auto c=box.local_center;
                inertia.x+=b.mass*(c.y*c.y+c.z*c.z);
                inertia.y+=b.mass*(c.x*c.x+c.z*c.z);
                inertia.z+=b.mass*(c.x*c.x+c.y*c.y);
            }
            b.inverse_inertia_local={1.0F/inertia.x,1.0F/inertia.y,1.0F/inertia.z};
        }
        return b;
    }
    [[nodiscard]] static Vec3 inverse_inertia_world(const Body& b,Vec3 torque) noexcept {
        const Vec3 local=inverse_rotate(torque,b.rotation);
        const Vec3 transformed{
            local.x*b.inverse_inertia_local.x,
            local.y*b.inverse_inertia_local.y,
            local.z*b.inverse_inertia_local.z};
        return rotate(transformed,b.rotation);
    }
    static void apply_impulse_to(Body& b,Vec3 impulse,Vec3 arm) noexcept {
        if(b.type!=BodyType3D::Dynamic) return;
        b.linear_velocity=b.linear_velocity+impulse*b.inverse_mass;
        b.angular_velocity=b.angular_velocity+
            inverse_inertia_world(b,cross(arm,impulse));
    }
    [[nodiscard]] static Quaternion3D integrate_rotation(
        Quaternion3D rotation,Vec3 angular_velocity,float dt) noexcept {
        const Quaternion3D omega{angular_velocity.x,angular_velocity.y,
            angular_velocity.z,0.0F};
        const auto derivative=multiply(omega,rotation);
        rotation.x+=0.5F*derivative.x*dt;
        rotation.y+=0.5F*derivative.y*dt;
        rotation.z+=0.5F*derivative.z*dt;
        rotation.w+=0.5F*derivative.w*dt;
        return normalized(rotation);
    }
    [[nodiscard]] static WorldShape world_shape(const Body& b) noexcept {
        if(const auto* sphere=std::get_if<SphereCollider3D>(&b.collider)) {
            return Sphere3D{b.position+rotate(sphere->local_center,b.rotation),sphere->radius};
        }
        const auto& box=std::get<BoxCollider3D>(b.collider);
        const auto rotation=normalized(multiply(b.rotation,box.local_rotation));
        return OrientedBox3D{
            b.position+rotate(box.local_center,b.rotation),box.half_extents,
            {{rotate({1,0,0},rotation),rotate({0,1,0},rotation),rotate({0,0,1},rotation)}}};
    }
    [[nodiscard]] static Aabb3D world_bounds(const Body& b) noexcept {
        return std::visit([](const auto& shape){return shape.bounds();},world_shape(b));
    }
    [[nodiscard]] bool update_proxy(const Body& b) {
        return broadphase_.upsert(proxy_id(b.handle),world_bounds(b));
    }
    void rebuild_broadphase() {
        broadphase_.clear();
        for(const auto& [index,body]:bodies_){(void)index;(void)update_proxy(body);}
    }
    [[nodiscard]] static bool should_collide(
        CollisionFilter3D a,CollisionFilter3D b) noexcept {
        if(a.group_index!=0&&a.group_index==b.group_index)
            return a.group_index>0;
        return (a.mask_bits&b.category_bits)!=0U&&
            (b.mask_bits&a.category_bits)!=0U;
    }
    [[nodiscard]] static std::optional<ContactManifold3D> collide_shapes(
        const WorldShape& a,const WorldShape& b) noexcept {
        return std::visit([](const auto& left,const auto& right)
            ->std::optional<ContactManifold3D> {
            using L=std::decay_t<decltype(left)>;
            using R=std::decay_t<decltype(right)>;
            if constexpr(std::is_same_v<L,Sphere3D>&&std::is_same_v<R,Sphere3D>)
                return collide(left,right);
            else if constexpr(std::is_same_v<L,Sphere3D>&&std::is_same_v<R,OrientedBox3D>)
                return collide(left,right);
            else if constexpr(std::is_same_v<L,OrientedBox3D>&&std::is_same_v<R,Sphere3D>)
                return collide(left,right);
            else return collide(left,right);
        },a,b);
    }
    [[nodiscard]] static Vec3 velocity_at(const Body& b,Vec3 point) noexcept {
        return b.linear_velocity+cross(b.angular_velocity,point-b.position);
    }
    [[nodiscard]] static float effective_mass(
        const Body& a,const Body& b,Vec3 ra,Vec3 rb,Vec3 axis) noexcept {
        const Vec3 ca=inverse_inertia_world(a,cross(ra,axis));
        const Vec3 cb=inverse_inertia_world(b,cross(rb,axis));
        return a.inverse_mass+b.inverse_mass+
            dot(cross(ca,ra),axis)+dot(cross(cb,rb),axis);
    }
    void solve_contacts(
        std::map<PairKey,PhysicsContact3D>& contacts,float dt) {
        for(auto& [key,contact]:contacts) {
            (void)key;
            if(contact.sensor) continue;
            const auto ia=bodies_.find(contact.first.index());
            const auto ib=bodies_.find(contact.second.index());
            if(ia==bodies_.end()||ib==bodies_.end()) continue;
            auto& a=ia->second;auto& b=ib->second;
            const Vec3 ra=contact.manifold.point-a.position;
            const Vec3 rb=contact.manifold.point-b.position;
            const Vec3 normal=normalized(contact.manifold.normal);
            const float normal_mass=effective_mass(a,b,ra,rb,normal);
            if(normal_mass<=1.0e-8F) continue;
            const float initial_speed=dot(
                velocity_at(b,contact.manifold.point)-
                velocity_at(a,contact.manifold.point),normal);
            const float restitution=initial_speed<-
                settings_.restitution_velocity_threshold
                ?std::max(a.restitution,b.restitution):0.0F;
            const float target_speed=-restitution*initial_speed;

            for(std::uint32_t iteration=0;iteration<settings_.velocity_iterations;++iteration) {
                Vec3 relative=velocity_at(b,contact.manifold.point)-
                    velocity_at(a,contact.manifold.point);
                const float normal_speed=dot(relative,normal);
                const float candidate=std::max(
                    0.0F,contact.normal_impulse+
                    (target_speed-normal_speed)/normal_mass);
                const float impulse=candidate-contact.normal_impulse;
                contact.normal_impulse=candidate;
                const Vec3 normal_impulse=normal*impulse;
                apply_impulse_to(a,-normal_impulse,ra);
                apply_impulse_to(b,normal_impulse,rb);

                relative=velocity_at(b,contact.manifold.point)-
                    velocity_at(a,contact.manifold.point);
                const Vec3 tangent_velocity=relative-normal*dot(relative,normal);
                const float tangent_length=length(tangent_velocity);
                if(tangent_length>1.0e-6F) {
                    const Vec3 tangent=tangent_velocity/tangent_length;
                    const float tangent_mass=effective_mass(a,b,ra,rb,tangent);
                    if(tangent_mass>1.0e-8F) {
                        const float coefficient=std::sqrt(a.friction*b.friction);
                        const float limit=contact.normal_impulse*coefficient;
                        const float candidate_tangent=std::clamp(
                            contact.tangent_impulse-
                                dot(relative,tangent)/tangent_mass,
                            -limit,limit);
                        const float friction_impulse=
                            candidate_tangent-contact.tangent_impulse;
                        contact.tangent_impulse=candidate_tangent;
                        const Vec3 applied=tangent*friction_impulse;
                        apply_impulse_to(a,-applied,ra);
                        apply_impulse_to(b,applied,rb);
                    }
                }
            }
        }

        for(std::uint32_t iteration=0;iteration<settings_.position_iterations;++iteration) {
            for(auto& [key,contact]:contacts) {
                (void)key;
                if(contact.sensor) continue;
                auto ia=bodies_.find(contact.first.index());
                auto ib=bodies_.find(contact.second.index());
                if(ia==bodies_.end()||ib==bodies_.end()) continue;
                auto& a=ia->second;auto& b=ib->second;
                const float inverse_sum=a.inverse_mass+b.inverse_mass;
                if(inverse_sum<=1.0e-8F) continue;
                const float penetration=std::max(
                    0.0F,contact.manifold.penetration-settings_.penetration_slop);
                const float magnitude=penetration*settings_.position_correction_percent/
                    (inverse_sum*static_cast<float>(settings_.position_iterations));
                const Vec3 correction=contact.manifold.normal*magnitude;
                if(a.type==BodyType3D::Dynamic)
                    a.position=a.position-correction*a.inverse_mass;
                if(b.type==BodyType3D::Dynamic)
                    b.position=b.position+correction*b.inverse_mass;
            }
        }
        (void)dt;
    }

    PhysicsSettings3D settings_{};
    bool settings_valid_{false};
    core::HandlePool<BodyTag3D> handles_;
    std::map<std::uint32_t,Body> bodies_;
    Broadphase3D broadphase_;
    std::map<PairKey,PhysicsContact3D> active_contacts_;
};

} // namespace dream::physics3d
