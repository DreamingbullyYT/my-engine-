#pragma once

#include <dream/core/handle.hpp>
#include <dream/world/transform.hpp>
#include <dream/world/world.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace dream::animation {

struct AnimationPlayerTag;
using AnimationPlayerHandle = core::Handle<AnimationPlayerTag>;

enum class LoopMode : std::uint8_t { Once, Loop, PingPong };

enum class AnimationError : std::uint8_t {
    None, InvalidDelta, InvalidClip, InvalidPlaybackSettings,
    InvalidHandle, CapacityExhausted
};

struct PlaybackSettings final {
    LoopMode loop_mode{LoopMode::Once};
    double playback_rate{1.0};
    bool start_playing{true};
};

template <typename Value>
struct Keyframe final {
    double time_seconds{0.0};
    Value value{};
};

namespace detail {

[[nodiscard]] inline bool finite(world::Vec2 v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y);
}
[[nodiscard]] inline bool finite(world::Vec3 v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
[[nodiscard]] inline bool finite(world::Quaternion q) noexcept {
    return std::isfinite(q.x)&&std::isfinite(q.y)&&
        std::isfinite(q.z)&&std::isfinite(q.w);
}
[[nodiscard]] inline bool finite(const world::Transform2D& v) noexcept {
    return finite(v.position)&&std::isfinite(v.rotation_radians)&&finite(v.scale);
}
[[nodiscard]] inline bool finite(const world::Transform3D& v) noexcept {
    return finite(v.position)&&finite(v.rotation)&&finite(v.scale);
}
[[nodiscard]] inline bool valid_rotation(world::Quaternion q) noexcept {
    if(!finite(q)) return false;
    const float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&n>1.0e-12F;
}
[[nodiscard]] inline world::Quaternion normalize(world::Quaternion q) noexcept {
    const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(!std::isfinite(n)||n<1.0e-8F) return {};
    return {q.x/n,q.y/n,q.z/n,q.w/n};
}
[[nodiscard]] inline float lerp(float a,float b,float t) noexcept {
    return a+(b-a)*t;
}
[[nodiscard]] inline world::Vec2 lerp(
    world::Vec2 a,world::Vec2 b,float t) noexcept {
    return {lerp(a.x,b.x,t),lerp(a.y,b.y,t)};
}
[[nodiscard]] inline world::Vec3 lerp(
    world::Vec3 a,world::Vec3 b,float t) noexcept {
    return {lerp(a.x,b.x,t),lerp(a.y,b.y,t),lerp(a.z,b.z,t)};
}
[[nodiscard]] inline world::Quaternion slerp(
    world::Quaternion a,world::Quaternion b,float t) noexcept {
    a=normalize(a);b=normalize(b);
    float cosine=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if(cosine<0.0F) {
        b={-b.x,-b.y,-b.z,-b.w};
        cosine=-cosine;
    }
    cosine=std::clamp(cosine,-1.0F,1.0F);
    if(cosine>0.9995F) {
        return normalize({
            lerp(a.x,b.x,t),lerp(a.y,b.y,t),
            lerp(a.z,b.z,t),lerp(a.w,b.w,t)});
    }
    const float angle=std::acos(cosine);
    const float sine=std::sin(angle);
    if(std::abs(sine)<1.0e-7F) return a;
    const float left=std::sin((1.0F-t)*angle)/sine;
    const float right=std::sin(t*angle)/sine;
    return normalize({
        a.x*left+b.x*right,a.y*left+b.y*right,
        a.z*left+b.z*right,a.w*left+b.w*right});
}
[[nodiscard]] inline float shortest_angle(float from,float to,float t) noexcept {
    constexpr float tau=6.2831853071795864769F;
    float delta=std::fmod(to-from,tau);
    if(delta>tau*0.5F) delta-=tau;
    else if(delta<-tau*0.5F) delta+=tau;
    return from+delta*t;
}
[[nodiscard]] inline world::Transform2D interpolate(
    const world::Transform2D& a,const world::Transform2D& b,float t) noexcept {
    return {
        lerp(a.position,b.position,t),
        shortest_angle(a.rotation_radians,b.rotation_radians,t),
        lerp(a.scale,b.scale,t)
    };
}
[[nodiscard]] inline world::Transform3D interpolate(
    const world::Transform3D& a,const world::Transform3D& b,float t) noexcept {
    return {
        lerp(a.position,b.position,t),
        slerp(a.rotation,b.rotation,t),
        lerp(a.scale,b.scale,t)
    };
}

template <typename Value>
[[nodiscard]] bool valid_keyframes(
    const std::vector<Keyframe<Value>>& frames,double duration) noexcept {
    if(frames.empty()||frames.size()>100000U) return false;
    double previous=-1.0;
    for(const auto& frame:frames) {
        if(!std::isfinite(frame.time_seconds)||frame.time_seconds<0.0||
           frame.time_seconds>duration||frame.time_seconds<=previous)
            return false;
        if(!finite(frame.value)) return false;
        if constexpr(std::is_same_v<Value,world::Transform3D>) {
            if(!valid_rotation(frame.value.rotation)) return false;
        }
        previous=frame.time_seconds;
    }
    return true;
}

template <typename Value>
[[nodiscard]] Value sample(
    const std::vector<Keyframe<Value>>& frames,double time_seconds) noexcept {
    if(time_seconds<=frames.front().time_seconds) return frames.front().value;
    if(time_seconds>=frames.back().time_seconds) return frames.back().value;
    const auto upper=std::upper_bound(frames.begin(),frames.end(),time_seconds,
        [](double time,const Keyframe<Value>& frame) {
            return time<frame.time_seconds;
        });
    const auto lower=upper-1;
    const double interval=upper->time_seconds-lower->time_seconds;
    const float alpha=static_cast<float>(
        (time_seconds-lower->time_seconds)/interval);
    return interpolate(lower->value,upper->value,alpha);
}

[[nodiscard]] inline bool valid_loop_mode(LoopMode mode) noexcept {
    switch(mode) {
    case LoopMode::Once: case LoopMode::Loop: case LoopMode::PingPong: return true;
    }
    return false;
}

} // namespace detail

struct TransformTrack2D final {
    world::Entity target{};
    std::vector<Keyframe<world::Transform2D>> keyframes;
};
struct TransformTrack3D final {
    world::Entity target{};
    std::vector<Keyframe<world::Transform3D>> keyframes;
};

struct AnimationClip2D final {
    std::string name;
    double duration_seconds{0.0};
    std::vector<TransformTrack2D> tracks;

    [[nodiscard]] bool valid() const noexcept {
        if(name.empty()||name.size()>128U||!std::isfinite(duration_seconds)||
           duration_seconds<=0.0||tracks.empty()||tracks.size()>10000U)
            return false;
        for(std::size_t i=0;i<tracks.size();++i) {
            if(!tracks[i].target||
               !detail::valid_keyframes(tracks[i].keyframes,duration_seconds))
                return false;
            for(std::size_t j=0;j<i;++j)
                if(tracks[j].target==tracks[i].target) return false;
        }
        return true;
    }
};
struct AnimationClip3D final {
    std::string name;
    double duration_seconds{0.0};
    std::vector<TransformTrack3D> tracks;

    [[nodiscard]] bool valid() const noexcept {
        if(name.empty()||name.size()>128U||!std::isfinite(duration_seconds)||
           duration_seconds<=0.0||tracks.empty()||tracks.size()>10000U)
            return false;
        for(std::size_t i=0;i<tracks.size();++i) {
            if(!tracks[i].target||
               !detail::valid_keyframes(tracks[i].keyframes,duration_seconds))
                return false;
            for(std::size_t j=0;j<i;++j)
                if(tracks[j].target==tracks[i].target) return false;
        }
        return true;
    }
};

struct AnimationPlayerState final {
    AnimationPlayerHandle handle{};
    double time_seconds{0.0};
    double playback_rate{1.0};
    LoopMode loop_mode{LoopMode::Once};
    bool playing{false};
    bool finished{false};
};

struct AnimationUpdateReport final {
    AnimationError error{AnimationError::None};
    std::size_t players_advanced{0};
    std::size_t transforms_written{0};
    std::size_t missing_targets{0};
    std::size_t players_finished{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error==AnimationError::None;
    }
};

// Thread-confined animation service. Create/remove/control players on the same
// simulation thread that calls update(). Each player owns a clip snapshot and
// applies Transform2D/Transform3D samples directly to World.
class AnimationRuntime final {
public:
    static constexpr std::size_t maximum_players=256U;

    AnimationRuntime()=default;
    AnimationRuntime(const AnimationRuntime&)=delete;
    AnimationRuntime& operator=(const AnimationRuntime&)=delete;

    [[nodiscard]] std::pair<AnimationPlayerHandle,AnimationError> play(
        AnimationClip2D clip,PlaybackSettings settings={}) {
        if(!clip.valid()) return {{},AnimationError::InvalidClip};
        if(!valid_settings(settings)) return {{},AnimationError::InvalidPlaybackSettings};
        return insert_player(Player2D{
            std::move(clip),0.0,settings.playback_rate,
            settings.loop_mode,settings.start_playing,false});
    }

    [[nodiscard]] std::pair<AnimationPlayerHandle,AnimationError> play(
        AnimationClip3D clip,PlaybackSettings settings={}) {
        if(!clip.valid()) return {{},AnimationError::InvalidClip};
        if(!valid_settings(settings)) return {{},AnimationError::InvalidPlaybackSettings};
        return insert_player(Player3D{
            std::move(clip),0.0,settings.playback_rate,
            settings.loop_mode,settings.start_playing,false});
    }

    [[nodiscard]] bool pause(AnimationPlayerHandle handle) noexcept {
        auto* player=find_player(handle);
        if(!player) return false;
        std::visit([](auto& value){value.playing=false;},*player);
        return true;
    }
    [[nodiscard]] bool resume(AnimationPlayerHandle handle) noexcept {
        auto* player=find_player(handle);
        if(!player) return false;
        std::visit([](auto& value) {
            if(value.finished) {
                value.time_seconds=0.0;
                value.finished=false;
            }
            value.playing=true;
        },*player);
        return true;
    }
    [[nodiscard]] bool stop(AnimationPlayerHandle handle) noexcept {
        auto* player=find_player(handle);
        if(!player) return false;
        std::visit([](auto& value) {
            value.time_seconds=0.0;
            value.playing=false;
            value.finished=false;
        },*player);
        return true;
    }
    [[nodiscard]] bool seek(AnimationPlayerHandle handle,double time_seconds) noexcept {
        if(!std::isfinite(time_seconds)) return false;
        auto* player=find_player(handle);
        if(!player) return false;
        std::visit([&](auto& value) {
            value.time_seconds=std::clamp(
                time_seconds,0.0,value.clip.duration_seconds);
            value.finished=false;
        },*player);
        return true;
    }
    [[nodiscard]] bool set_playback_rate(
        AnimationPlayerHandle handle,double rate) noexcept {
        if(!std::isfinite(rate)||rate<0.05||rate>8.0) return false;
        auto* player=find_player(handle);
        if(!player) return false;
        std::visit([&](auto& value){value.playback_rate=rate;},*player);
        return true;
    }
    [[nodiscard]] bool remove(AnimationPlayerHandle handle) {
        if(!find_player(handle)||!handles_.release(handle)) return false;
        players_.erase(handle.index());
        return true;
    }
    [[nodiscard]] std::optional<AnimationPlayerState> state(
        AnimationPlayerHandle handle) const noexcept {
        const auto* player=find_player(handle);
        if(!player) return std::nullopt;
        return std::visit([&](const auto& value) {
            return AnimationPlayerState{
                value.handle,value.time_seconds,value.playback_rate,
                value.loop_mode,value.playing,value.finished};
        },*player);
    }
    [[nodiscard]] std::size_t player_count() const noexcept {
        return players_.size();
    }

    [[nodiscard]] AnimationUpdateReport update(
        world::World& world,double delta_seconds) {
        AnimationUpdateReport report;
        if(!std::isfinite(delta_seconds)||delta_seconds<0.0) {
            report.error=AnimationError::InvalidDelta;
            return report;
        }
        for(auto& [index,player]:players_) {
            (void)index;
            std::visit([&](auto& value) {
                if(!value.playing) return;
                advance(value,delta_seconds,report);
                apply(world,value,report);
                ++report.players_advanced;
            },player);
        }
        return report;
    }

private:
    struct PlayerBase final {
        AnimationPlayerHandle handle{};
        double time_seconds{0.0};
        double playback_rate{1.0};
        LoopMode loop_mode{LoopMode::Once};
        bool playing{true};
        bool finished{false};
    };
    struct Player2D final : PlayerBase {
        AnimationClip2D clip;
        Player2D(AnimationClip2D c,double t,double rate,LoopMode mode,
                 bool active,bool done)
            :clip(std::move(c)) {
            time_seconds=t;playback_rate=rate;loop_mode=mode;
            playing=active;finished=done;
        }
    };
    struct Player3D final : PlayerBase {
        AnimationClip3D clip;
        Player3D(AnimationClip3D c,double t,double rate,LoopMode mode,
                 bool active,bool done)
            :clip(std::move(c)) {
            time_seconds=t;playback_rate=rate;loop_mode=mode;
            playing=active;finished=done;
        }
    };
    using Player=std::variant<Player2D,Player3D>;

    [[nodiscard]] static bool valid_settings(PlaybackSettings settings) noexcept {
        return detail::valid_loop_mode(settings.loop_mode)&&
            std::isfinite(settings.playback_rate)&&
            settings.playback_rate>=0.05&&settings.playback_rate<=8.0;
    }
    template <typename PlayerType>
    [[nodiscard]] std::pair<AnimationPlayerHandle,AnimationError> insert_player(
        PlayerType player) {
        if(players_.size()>=maximum_players)
            return {{},AnimationError::CapacityExhausted};
        const auto handle=handles_.allocate();
        if(!handle) return {{},AnimationError::CapacityExhausted};
        player.handle=handle;
        try {
            const auto [it,inserted]=players_.emplace(
                handle.index(),Player{std::move(player)});
            (void)it;
            if(!inserted) {
                (void)handles_.release(handle);
                return {{},AnimationError::CapacityExhausted};
            }
        } catch(...) {
            (void)handles_.release(handle);
            throw;
        }
        return {handle,AnimationError::None};
    }
    [[nodiscard]] Player* find_player(AnimationPlayerHandle handle) noexcept {
        if(!handle) return nullptr;
        const auto it=players_.find(handle.index());
        if(it==players_.end()) return nullptr;
        const auto stored=std::visit(
            [](const auto& value){return value.handle;},it->second);
        return stored==handle?&it->second:nullptr;
    }
    [[nodiscard]] const Player* find_player(
        AnimationPlayerHandle handle) const noexcept {
        if(!handle) return nullptr;
        const auto it=players_.find(handle.index());
        if(it==players_.end()) return nullptr;
        const auto stored=std::visit(
            [](const auto& value){return value.handle;},it->second);
        return stored==handle?&it->second:nullptr;
    }

    template <typename PlayerType>
    static void advance(
        PlayerType& player,double delta,AnimationUpdateReport& report) noexcept {
        if(delta==0.0) return;
        const double duration=player.clip.duration_seconds;
        const double next=player.time_seconds+delta*player.playback_rate;
        switch(player.loop_mode) {
        case LoopMode::Once:
            player.time_seconds=std::clamp(next,0.0,duration);
            if(next>=duration) {
                player.playing=false;
                player.finished=true;
                ++report.players_finished;
            }
            break;
        case LoopMode::Loop:
            player.time_seconds=std::fmod(next,duration);
            break;
        case LoopMode::PingPong: {
            const double period=duration*2.0;
            const double phase=std::fmod(next,period);
            player.time_seconds=phase<=duration?phase:period-phase;
            break;
        }
        }
    }

    static void apply(
        world::World& world,const Player2D& player,
        AnimationUpdateReport& report) {
        for(const auto& track:player.clip.tracks) {
            const auto value=detail::sample(track.keyframes,player.time_seconds);
            if(world.with_component_mut<world::Transform2D>(
                track.target,[&](world::Transform2D& transform){transform=value;})) {
                ++report.transforms_written;
            } else {
                ++report.missing_targets;
            }
        }
    }
    static void apply(
        world::World& world,const Player3D& player,
        AnimationUpdateReport& report) {
        for(const auto& track:player.clip.tracks) {
            const auto value=detail::sample(track.keyframes,player.time_seconds);
            if(world.with_component_mut<world::Transform3D>(
                track.target,[&](world::Transform3D& transform){transform=value;})) {
                ++report.transforms_written;
            } else {
                ++report.missing_targets;
            }
        }
    }

    core::HandlePool<AnimationPlayerTag> handles_;
    std::map<std::uint32_t,Player> players_;
};

} // namespace dream::animation
