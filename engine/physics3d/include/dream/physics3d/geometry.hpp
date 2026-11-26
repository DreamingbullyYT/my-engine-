#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace dream::physics3d {

struct Vec3 final { float x{0.0F}; float y{0.0F}; float z{0.0F}; };

[[nodiscard]] constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x+b.x,a.y+b.y,a.z+b.z};
}
[[nodiscard]] constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x-b.x,a.y-b.y,a.z-b.z};
}
[[nodiscard]] constexpr Vec3 operator-(Vec3 v) noexcept { return {-v.x,-v.y,-v.z}; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 v,float s) noexcept { return {v.x*s,v.y*s,v.z*s}; }
[[nodiscard]] constexpr Vec3 operator/(Vec3 v,float s) noexcept { return {v.x/s,v.y/s,v.z/s}; }
[[nodiscard]] constexpr float dot(Vec3 a,Vec3 b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
[[nodiscard]] constexpr Vec3 cross(Vec3 a,Vec3 b) noexcept {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
[[nodiscard]] inline float length_squared(Vec3 v) noexcept { return dot(v,v); }
[[nodiscard]] inline float length(Vec3 v) noexcept { return std::sqrt(length_squared(v)); }
[[nodiscard]] inline bool finite(Vec3 v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
[[nodiscard]] inline Vec3 normalized(Vec3 v) noexcept {
    const float n=length(v);
    return n>1.0e-8F?v/n:Vec3{1.0F,0.0F,0.0F};
}

struct Aabb3D final {
    Vec3 minimum{};
    Vec3 maximum{};

    [[nodiscard]] static Aabb3D from_center_half_extents(Vec3 c,Vec3 h) noexcept {
        return {c-h,c+h};
    }
    [[nodiscard]] Vec3 center() const noexcept { return (minimum+maximum)*0.5F; }
    [[nodiscard]] Vec3 half_extents() const noexcept { return (maximum-minimum)*0.5F; }
    [[nodiscard]] bool valid() const noexcept {
        return finite(minimum)&&finite(maximum)&&minimum.x<=maximum.x&&
            minimum.y<=maximum.y&&minimum.z<=maximum.z;
    }
};

struct Sphere3D final {
    Vec3 center{};
    float radius{0.5F};
    [[nodiscard]] bool valid() const noexcept {
        return finite(center)&&std::isfinite(radius)&&radius>=0.0F;
    }
    [[nodiscard]] Aabb3D bounds() const noexcept {
        const Vec3 r{radius,radius,radius};
        return {center-r,center+r};
    }
};

struct OrientedBox3D final {
    Vec3 center{};
    Vec3 half_extents{0.5F,0.5F,0.5F};
    // Orthonormal world-space local axes. Axis order is local X, Y, Z.
    std::array<Vec3,3> axes{{{1,0,0},{0,1,0},{0,0,1}}};

    [[nodiscard]] bool valid() const noexcept {
        constexpr float tolerance=1.0e-3F;
        if(!finite(center)||!finite(half_extents)||half_extents.x<0.0F||
           half_extents.y<0.0F||half_extents.z<0.0F) return false;
        for(const auto axis:axes)
            if(!finite(axis)||std::abs(length_squared(axis)-1.0F)>tolerance) return false;
        return std::abs(dot(axes[0],axes[1]))<=tolerance&&
            std::abs(dot(axes[0],axes[2]))<=tolerance&&
            std::abs(dot(axes[1],axes[2]))<=tolerance;
    }
    [[nodiscard]] Vec3 support(Vec3 direction) const noexcept {
        Vec3 point=center;
        point=point+axes[0]*(dot(direction,axes[0])>=0.0F?half_extents.x:-half_extents.x);
        point=point+axes[1]*(dot(direction,axes[1])>=0.0F?half_extents.y:-half_extents.y);
        point=point+axes[2]*(dot(direction,axes[2])>=0.0F?half_extents.z:-half_extents.z);
        return point;
    }
    [[nodiscard]] Aabb3D bounds() const noexcept {
        const Vec3 extent{
            std::abs(axes[0].x)*half_extents.x+std::abs(axes[1].x)*half_extents.y+std::abs(axes[2].x)*half_extents.z,
            std::abs(axes[0].y)*half_extents.x+std::abs(axes[1].y)*half_extents.y+std::abs(axes[2].y)*half_extents.z,
            std::abs(axes[0].z)*half_extents.x+std::abs(axes[1].z)*half_extents.y+std::abs(axes[2].z)*half_extents.z
        };
        return {center-extent,center+extent};
    }
};

struct ContactManifold3D final {
    // Points from the first shape toward the second. Move shape A opposite
    // this normal to resolve penetration.
    Vec3 normal{1.0F,0.0F,0.0F};
    Vec3 point{};
    float penetration{0.0F};
};

[[nodiscard]] inline ContactManifold3D flipped(ContactManifold3D m) noexcept {
    m.normal=-m.normal; return m;
}
[[nodiscard]] inline bool intersects(const Aabb3D& a,const Aabb3D& b) noexcept {
    return a.valid()&&b.valid()&&a.minimum.x<=b.maximum.x&&a.maximum.x>=b.minimum.x&&
        a.minimum.y<=b.maximum.y&&a.maximum.y>=b.minimum.y&&
        a.minimum.z<=b.maximum.z&&a.maximum.z>=b.minimum.z;
}
[[nodiscard]] inline std::optional<ContactManifold3D> collide(
    const Aabb3D& a,const Aabb3D& b) noexcept {
    if(!a.valid()||!b.valid()) return std::nullopt;
    const Vec3 overlap{
        std::min(a.maximum.x,b.maximum.x)-std::max(a.minimum.x,b.minimum.x),
        std::min(a.maximum.y,b.maximum.y)-std::max(a.minimum.y,b.minimum.y),
        std::min(a.maximum.z,b.maximum.z)-std::max(a.minimum.z,b.minimum.z)
    };
    if(overlap.x<0.0F||overlap.y<0.0F||overlap.z<0.0F) return std::nullopt;
    const Vec3 ca=a.center(),cb=b.center();
    ContactManifold3D m;
    if(overlap.x<=overlap.y&&overlap.x<=overlap.z) {
        const float sign=cb.x>=ca.x?1.0F:-1.0F;
        m.normal={sign,0,0}; m.penetration=overlap.x;
        m.point={(sign>0?(a.maximum.x+b.minimum.x):(a.minimum.x+b.maximum.x))*0.5F,
            (std::max(a.minimum.y,b.minimum.y)+std::min(a.maximum.y,b.maximum.y))*0.5F,
            (std::max(a.minimum.z,b.minimum.z)+std::min(a.maximum.z,b.maximum.z))*0.5F};
    } else if(overlap.y<=overlap.z) {
        const float sign=cb.y>=ca.y?1.0F:-1.0F;
        m.normal={0,sign,0}; m.penetration=overlap.y;
        m.point={(std::max(a.minimum.x,b.minimum.x)+std::min(a.maximum.x,b.maximum.x))*0.5F,
            (sign>0?(a.maximum.y+b.minimum.y):(a.minimum.y+b.maximum.y))*0.5F,
            (std::max(a.minimum.z,b.minimum.z)+std::min(a.maximum.z,b.maximum.z))*0.5F};
    } else {
        const float sign=cb.z>=ca.z?1.0F:-1.0F;
        m.normal={0,0,sign}; m.penetration=overlap.z;
        m.point={(std::max(a.minimum.x,b.minimum.x)+std::min(a.maximum.x,b.maximum.x))*0.5F,
            (std::max(a.minimum.y,b.minimum.y)+std::min(a.maximum.y,b.maximum.y))*0.5F,
            (sign>0?(a.maximum.z+b.minimum.z):(a.minimum.z+b.maximum.z))*0.5F};
    }
    return m;
}
[[nodiscard]] inline std::optional<ContactManifold3D> collide(
    const Sphere3D& a,const Sphere3D& b) noexcept {
    if(!a.valid()||!b.valid()) return std::nullopt;
    const Vec3 delta=b.center-a.center;
    const float d2=length_squared(delta),r=a.radius+b.radius;
    if(d2>r*r) return std::nullopt;
    const float d=std::sqrt(std::max(0.0F,d2));
    const Vec3 n=d>1.0e-8F?delta/d:Vec3{1,0,0};
    return ContactManifold3D{n,(a.center+n*a.radius+b.center-n*b.radius)*0.5F,
        std::max(0.0F,r-d)};
}
[[nodiscard]] inline std::optional<ContactManifold3D> collide(
    const Sphere3D& sphere,const Aabb3D& box) noexcept {
    if(!sphere.valid()||!box.valid()) return std::nullopt;
    const Vec3 closest{
        std::clamp(sphere.center.x,box.minimum.x,box.maximum.x),
        std::clamp(sphere.center.y,box.minimum.y,box.maximum.y),
        std::clamp(sphere.center.z,box.minimum.z,box.maximum.z)
    };
    const Vec3 delta=closest-sphere.center;
    const float d2=length_squared(delta);
    if(d2>sphere.radius*sphere.radius) return std::nullopt;
    if(d2>1.0e-12F) {
        const float d=std::sqrt(d2);
        return ContactManifold3D{delta/d,closest,std::max(0.0F,sphere.radius-d)};
    }
    const std::array<float,6> distances{
        sphere.center.x-box.minimum.x,box.maximum.x-sphere.center.x,
        sphere.center.y-box.minimum.y,box.maximum.y-sphere.center.y,
        sphere.center.z-box.minimum.z,box.maximum.z-sphere.center.z
    };
    const auto face=static_cast<std::size_t>(
        std::min_element(distances.begin(),distances.end())-distances.begin());
    Vec3 outward{},point=sphere.center;
    switch(face) {
    case 0: outward={-1,0,0}; point.x=box.minimum.x; break;
    case 1: outward={1,0,0}; point.x=box.maximum.x; break;
    case 2: outward={0,-1,0}; point.y=box.minimum.y; break;
    case 3: outward={0,1,0}; point.y=box.maximum.y; break;
    case 4: outward={0,0,-1}; point.z=box.minimum.z; break;
    default: outward={0,0,1}; point.z=box.maximum.z; break;
    }
    return ContactManifold3D{-outward,point,sphere.radius+distances[face]};
}
[[nodiscard]] inline std::optional<ContactManifold3D> collide(
    const Aabb3D& box,const Sphere3D& sphere) noexcept {
    auto result=collide(sphere,box); if(result) *result=flipped(*result); return result;
}
[[nodiscard]] inline std::optional<ContactManifold3D> collide(
    const OrientedBox3D& a,const OrientedBox3D& b) noexcept {
    if(!a.valid()||!b.valid()) return std::nullopt;
    const Vec3 delta=b.center-a.center;
    float minimum_overlap=std::numeric_limits<float>::max();
    Vec3 best_axis{1,0,0};
    const auto test_axis=[&](Vec3 axis)->bool {
        const float n2=length_squared(axis);
        if(n2<1.0e-10F) return true;
        axis=axis/std::sqrt(n2);
        const float ra=a.half_extents.x*std::abs(dot(axis,a.axes[0]))+
            a.half_extents.y*std::abs(dot(axis,a.axes[1]))+
            a.half_extents.z*std::abs(dot(axis,a.axes[2]));
        const float rb=b.half_extents.x*std::abs(dot(axis,b.axes[0]))+
            b.half_extents.y*std::abs(dot(axis,b.axes[1]))+
            b.half_extents.z*std::abs(dot(axis,b.axes[2]));
        const float distance=std::abs(dot(delta,axis));
        const float overlap=ra+rb-distance;
        if(overlap<0.0F) return false;
        if(overlap<minimum_overlap) {
            minimum_overlap=overlap;
            best_axis=dot(delta,axis)>=0.0F?axis:-axis;
        }
        return true;
    };
    for(const auto axis:a.axes) if(!test_axis(axis)) return std::nullopt;
    for(const auto axis:b.axes) if(!test_axis(axis)) return std::nullopt;
    for(const auto axis_a:a.axes) for(const auto axis_b:b.axes)
        if(!test_axis(cross(axis_a,axis_b))) return std::nullopt;
    const Vec3 point=(a.support(best_axis)+b.support(-best_axis))*0.5F;
    return ContactManifold3D{best_axis,point,minimum_overlap};
}

} // namespace dream::physics3d
