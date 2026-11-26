#include <dream/physics3d/physics_world.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
using namespace dream::physics3d;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
bool near(float a,float b,float e=0.03F){return std::abs(a-b)<=e;}
}
int main() {
    PhysicsWorld3D invalid(PhysicsSettings3D{.velocity_iterations=0});
    require(!invalid.valid()&&invalid.update(0.01).error==PhysicsStepError3D::InvalidSettings,
        "invalid settings reject simulation");

    PhysicsWorld3D world(PhysicsSettings3D{
        .gravity={0,-9.81F,0},.velocity_iterations=10,
        .position_iterations=4,.penetration_slop=0.002F,
        .position_correction_percent=0.85F});
    BodyDescription3D ground_desc;
    ground_desc.type=BodyType3D::Static;
    ground_desc.position={0,-0.5F,0};
    ground_desc.friction=0.8F;
    ground_desc.collider=BoxCollider3D{{},{5,0.5F,5},{}};
    const auto ground=world.create_body(ground_desc);
    BodyDescription3D ball_desc;
    ball_desc.type=BodyType3D::Dynamic;
    ball_desc.position={0,2,0};
    ball_desc.mass=1.0F;
    ball_desc.friction=0.8F;
    ball_desc.collider=SphereCollider3D{{},0.5F};
    const auto ball=world.create_body(ball_desc);
    require(ground&&ball,"static box and dynamic sphere bodies create");
    require(world.body_count()==2&&world.broadphase().proxy_count()==2,
        "body creation registers broadphase proxies");

    std::size_t total_contacts=0;
    for(int frame=0;frame<240;++frame) {
        const auto result=world.update(1.0/60.0);
        require(static_cast<bool>(result),"3D fixed step completes");
        total_contacts+=result.contact_count;
    }
    auto state=world.body_state(ball.handle);
    require(state&&state->position.y>0.43F&&state->position.y<0.62F,
        "gravity and 3D contact solver settle sphere on ground");
    require(std::abs(state->linear_velocity.y)<0.3F&&total_contacts>0,
        "contact impulse removes most vertical speed");

    require(world.set_velocity(ball.handle,{4,0,0}),"dynamic velocity can be assigned");
    const auto friction=world.update(1.0/60.0);
    require(friction&&friction.contact_count>0,
        "resting body continues to report its contact");
    require(std::abs(world.body_state(ball.handle)->linear_velocity.x)<4.0F,
        "contact friction reduces tangential sliding");

    require(world.apply_impulse(ball.handle,{0,0,2},state->position),
        "dynamic body accepts an impulse");
    require(world.body_state(ball.handle)->linear_velocity.z>0.0F,
        "impulse changes linear velocity");

    const auto stale=ball.handle;
    require(world.destroy_body(stale)&&!world.body_state(stale),
        "body destruction invalidates its generational handle");
    const auto reused=world.create_body(ball_desc);
    require(reused&&reused.handle.index()==stale.index()&&
            reused.handle.generation()!=stale.generation(),
        "body slot reuse advances its generation");

    PhysicsWorld3D sensors(PhysicsSettings3D{.gravity={0,0,0}});
    BodyDescription3D sensor_desc;
    sensor_desc.type=BodyType3D::Static;
    sensor_desc.sensor=true;
    sensor_desc.collider=SphereCollider3D{{},1.0F};
    BodyDescription3D overlap_desc;
    overlap_desc.type=BodyType3D::Dynamic;
    overlap_desc.position={1,0,0};
    overlap_desc.mass=1.0F;
    overlap_desc.collider=SphereCollider3D{{},1.0F};
    const auto sensor=sensors.create_body(sensor_desc);
    const auto overlapping=sensors.create_body(overlap_desc);
    require(sensor&&overlapping,"sensor pair bodies create");
    auto events=sensors.update(1.0/60.0);
    require(events&&events.contact_count==1&&events.events.size()==1&&
            events.events[0].type==ContactEventType3D::Begin&&
            events.events[0].contact.sensor,
        "sensor reports Begin without physical response");
    events=sensors.update(1.0/60.0);
    require(events.events.size()==1&&events.events[0].type==ContactEventType3D::Stay,
        "persistent sensor overlap reports Stay");
    require(sensors.set_velocity(overlapping.handle,{120,0,0}),
        "sensor body can be moved by setting velocity");
    events=sensors.update(1.0/60.0);
    require(events.events.size()==1&&events.events[0].type==ContactEventType3D::End,
        "leaving a sensor reports End");

    PhysicsWorld3D filters(PhysicsSettings3D{.gravity={0,0,0}});
    BodyDescription3D a_desc;
    a_desc.type=BodyType3D::Dynamic;a_desc.mass=1;
    a_desc.collider=SphereCollider3D{{},1};
    a_desc.collision_filter={0x1U,0x2U,0};
    BodyDescription3D b_desc;
    b_desc.position={1,0,0};b_desc.collider=SphereCollider3D{{},1};
    b_desc.collision_filter={0x2U,0x4U,0};
    const auto fa=filters.create_body(a_desc);
    const auto fb=filters.create_body(b_desc);
    require(fa&&fb&&filters.update(1.0/60.0).contact_count==0,
        "collision requires mutual category/mask permission");

    require(world.update(-0.1).error==PhysicsStepError3D::InvalidDelta,
        "negative delta is rejected");
    require(world.update(0.251).error==PhysicsStepError3D::DeltaTooLarge,
        "oversized delta is rejected");
    return EXIT_SUCCESS;
}
