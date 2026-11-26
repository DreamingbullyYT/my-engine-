#pragma once

#include <dream/world/system_scheduler.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace dream::world {

// A Scene owns an independent entity world and system schedule.
class Scene final {
public:
    explicit Scene(std::string name) : name_(std::move(name)) {
        if (name_.empty() || name_.size() > 256)
            throw std::invalid_argument("Scene name must contain 1 to 256 bytes");
    }

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] World& world() noexcept { return world_; }
    [[nodiscard]] const World& world() const noexcept { return world_; }
    [[nodiscard]] SystemScheduler& systems() noexcept { return systems_; }
    [[nodiscard]] const SystemScheduler& systems() const noexcept { return systems_; }

    [[nodiscard]] SystemUpdateReport update(float delta_seconds) {
        return systems_.update(world_, delta_seconds);
    }

private:
    std::string name_;
    World world_;
    SystemScheduler systems_;
};

} // namespace dream::world
