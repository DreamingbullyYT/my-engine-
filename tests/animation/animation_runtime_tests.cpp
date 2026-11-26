#include <dream/animation/animation_runtime.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {
using namespace dream::animation;
using namespace dream::world;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
bool near(float a,float b,float e=1.0e-3F){return std::abs(a-b)<=e;}
AnimationClip2D clip2d(Entity target) {
    AnimationClip2D clip;
    clip.name="move";
    clip.duration_seconds=2.0;
    clip.tracks.push_back({target,{
        {0.0,Transform2D{{0,0},0,{1,1}}},
        {1.0,Transform2D{{10,4},1.0F,{2,2}}},
        {2.0,Transform2D{{20,8},2.0F,{3,3}}}
    }});
    return clip;
}
}

int main() {
    World world;
    const Entity entity=world.create_entity();
    require(static_cast<bool>(entity),"entity creates");
    require(world.emplace_component<Transform2D>(entity,Transform2D{}),
        "2D transform component inserts");

    AnimationRuntime runtime;
    auto created=runtime.play(clip2d(entity),PlaybackSettings{
        .loop_mode=LoopMode::Once,.playback_rate=1.0,.start_playing=true});
    require(created.second==AnimationError::None&&static_cast<bool>(created.first),
        "valid 2D animation player is created");

    auto report=runtime.update(world,0.5);
    require(report&&report.players_advanced==1&&report.transforms_written==1,
        "player samples and applies its transform track");
    require(world.with_component<Transform2D>(entity,[&](const Transform2D& t) {
        require(near(t.position.x,5.0F)&&near(t.position.y,2.0F),
            "linear position interpolation is correct");
        require(near(t.scale.x,1.5F)&&near(t.rotation_radians,0.5F),
            "2D scale and rotation interpolation are correct");
    }),"animated transform remains accessible");

    require(runtime.seek(created.first,1.5),"player seeks to an exact sample time");
    report=runtime.update(world,0.0);
    require(report&&report.transforms_written==1,
        "zero delta can apply the current seek position");
    require(world.with_component<Transform2D>(entity,[&](const Transform2D& t) {
        require(near(t.position.x,15.0F),"seek samples between keyframes");
    }),"2D seek transform sampled");

    require(runtime.pause(created.first),"player pauses");
    report=runtime.update(world,0.25);
    require(report.players_advanced==0,"paused player does not advance");
    require(runtime.resume(created.first),"player resumes");
    report=runtime.update(world,1.0);
    require(report.players_finished==1&&!runtime.state(created.first)->playing,
        "once playback finishes exactly at clip duration");
    require(near(static_cast<float>(runtime.state(created.first)->time_seconds),2.0F),
        "once playback clamps at clip duration");

    auto loop=runtime.play(clip2d(entity),PlaybackSettings{
        .loop_mode=LoopMode::Loop,.playback_rate=1.0,.start_playing=true});
    require(loop.second==AnimationError::None,"looping player creates");
    report=runtime.update(world,2.5);
    require(report&&near(static_cast<float>(runtime.state(loop.first)->time_seconds),0.5F),
        "looping playback wraps time without drift");

    auto ping=runtime.play(clip2d(entity),PlaybackSettings{
        .loop_mode=LoopMode::PingPong,.playback_rate=1.0,.start_playing=true});
    require(ping.second==AnimationError::None,"ping-pong player creates");
    report=runtime.update(world,2.5);
    require(report&&near(static_cast<float>(runtime.state(ping.first)->time_seconds),1.5F),
        "ping-pong playback reflects at the end");

    const Entity entity3d=world.create_entity();
    require(world.emplace_component<Transform3D>(entity3d,Transform3D{}),
        "3D transform component inserts");
    AnimationClip3D clip3d;
    clip3d.name="rotate";
    clip3d.duration_seconds=1.0;
    clip3d.tracks.push_back({entity3d,{
        {0.0,Transform3D{{0,0,0},{0,0,0,1},{1,1,1}}},
        {1.0,Transform3D{{0,2,0},{0,0.7071068F,0,0.7071068F},{2,2,2}}}
    }});
    auto player3d=runtime.play(std::move(clip3d));
    require(player3d.second==AnimationError::None,
        "3D transform animation player creates");
    report=runtime.update(world,0.5);
    require(report&&report.transforms_written>=1,
        "3D transform track applies to the World");
    require(world.with_component<Transform3D>(entity3d,
        [&](const Transform3D& transform) {
            require(near(transform.position.y,1.0F)&&near(transform.scale.x,1.5F),
                "3D position and scale interpolate");
            const float norm=std::sqrt(
                transform.rotation.x*transform.rotation.x+
                transform.rotation.y*transform.rotation.y+
                transform.rotation.z*transform.rotation.z+
                transform.rotation.w*transform.rotation.w);
            require(near(norm,1.0F),"quaternion interpolation stays normalized");
        }),"3D transform is readable");

    AnimationClip2D invalid=clip2d(entity);
    invalid.tracks[0].keyframes[1].time_seconds=0.0;
    require(runtime.play(std::move(invalid)).second==AnimationError::InvalidClip,
        "unordered/duplicate keyframe times are rejected");
    require(runtime.play(clip2d({})).second==AnimationError::InvalidClip,
        "invalid target handles are rejected");
    require(!runtime.set_playback_rate(created.first,20.0),
        "out-of-range playback rate is rejected");
    require(!runtime.update(world,-0.1),"negative delta is rejected");
    require(runtime.remove(created.first)&&!runtime.state(created.first),
        "player removal invalidates the handle");
    require(!runtime.remove(created.first),"duplicate removal is rejected");
    return EXIT_SUCCESS;
}
