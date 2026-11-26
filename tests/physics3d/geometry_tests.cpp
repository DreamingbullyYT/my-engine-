#include <dream/physics3d/geometry.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
using namespace dream::physics3d;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
bool near(float a,float b,float e=1.0e-3F){return std::abs(a-b)<=e;}
}
int main(){
    const Aabb3D a=Aabb3D::from_center_half_extents({0,0,0},{1,1,1});
    const Aabb3D b=Aabb3D::from_center_half_extents({1.5F,0,0},{1,1,1});
    const auto ab=collide(a,b);
    require(ab&&near(ab->penetration,0.5F)&&near(ab->normal.x,1.0F),
        "AABB collision reports minimum-axis penetration and A-to-B normal");
    require(!collide(a,Aabb3D::from_center_half_extents({3,0,0},{1,1,1})),
        "separated AABBs do not collide");
    require(intersects(a,b),"AABB broad overlap test detects overlapping extents");

    const Sphere3D s1{{0,0,0},1.0F},s2{{1.5F,0,0},1.0F};
    const auto ss=collide(s1,s2);
    require(ss&&near(ss->penetration,0.5F)&&near(ss->normal.x,1.0F),
        "sphere-sphere manifold is correct");
    require(!collide(s1,Sphere3D{{3,0,0},1.0F}),
        "separated spheres do not collide");

    const auto outside=collide(Sphere3D{{1.5F,0,0},1.0F},a);
    require(outside&&near(outside->penetration,0.5F)&&near(outside->normal.x,-1.0F),
        "sphere-AABB collision handles exterior contact");
    const auto inside=collide(Sphere3D{{0,0,0},0.25F},a);
    require(inside&&inside->penetration>0.25F&&near(length(inside->normal),1.0F),
        "sphere inside AABB selects a finite nearest exit face");

    const OrientedBox3D rotated_box{{0,0,0},{1.0F,0.2F,0.2F},
        {{{0.7071067F,0.7071067F,0},{-0.7071067F,0.7071067F,0},{0,0,1}}}};
    const auto sphere_obb=collide(Sphere3D{{1.0F,1.0F,0},0.5F},rotated_box);
    require(sphere_obb&&sphere_obb->penetration>0.0F,
        "sphere-OBB collision uses the box's rotated local axes");
    const auto obb_sphere=collide(rotated_box,Sphere3D{{1.0F,1.0F,0},0.5F});
    require(obb_sphere&&near(obb_sphere->normal.x,-sphere_obb->normal.x)&&
            near(obb_sphere->normal.y,-sphere_obb->normal.y),
        "reversed sphere-OBB query flips the contact normal");

    const OrientedBox3D obb_a{{0,0,0},{1,1,1}};
    const OrientedBox3D obb_b{{1.5F,0,0},{1,1,1}};
    const auto oo=collide(obb_a,obb_b);
    require(oo&&near(oo->penetration,0.5F)&&near(oo->normal.x,1.0F),
        "OBB SAT tests face axes and reports penetration");
    require(!collide(obb_a,OrientedBox3D{{3,0,0},{1,1,1}}),
        "separated OBBs do not collide");

    OrientedBox3D rotated{{1.4F,1.4F,0},{1.0F,0.2F,0.2F},
        {{{0.7071067F,0.7071067F,0},{-0.7071067F,0.7071067F,0},{0,0,1}}}};
    require(rotated.valid(),"orthonormal rotated box is accepted");
    require(!OrientedBox3D{{},{1,1,1},{{{2,0,0},{0,1,0},{0,0,1}}}}.valid(),
        "non-unit OBB axes are rejected");
    require(!Sphere3D{{},-1.0F}.valid(),"negative sphere radius is rejected");
    require(!Aabb3D{{1,0,0},{0,1,1}}.valid(),"inverted AABB is rejected");
    return EXIT_SUCCESS;
}
