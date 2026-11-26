#pragma once

#include <dream/world/system_scheduler.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace dream::world {

// A Scene owns an independent entity world and system schedule.
class Scene final {
public:
    explicit Scene(std::string name)
        : Scene(std::move(name), std::make_unique<World>()) {}

    // Used by validated scene restoration to install a fully built World.
    Scene(std::string name, std::unique_ptr<World> world)
        : name_(std::move(name)), world_(std::move(world)) {
        if (name_.empty() || name_.size() > 256)
            throw std::invalid_argument("Scene name must contain 1 to 256 bytes");
        if (!world_)
            throw std::invalid_argument("Scene requires a valid World");
    }

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] World& world() noexcept { return *world_; }
    [[nodiscard]] const World& world() const noexcept { return *world_; }
    [[nodiscard]] SystemScheduler& systems() noexcept { return systems_; }
    [[nodiscard]] const SystemScheduler& systems() const noexcept { return systems_; }

    [[nodiscard]] SystemUpdateReport update(float delta_seconds) {
        return systems_.update(*world_, delta_seconds);
    }

private:
    std::string name_;
    std::unique_ptr<World> world_;
    SystemScheduler systems_;
};

} // namespace dream::world
