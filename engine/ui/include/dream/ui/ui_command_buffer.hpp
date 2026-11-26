#pragma once

#include <dream/ui/ui_runtime.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dream::ui {

enum class UiDrawCommandType : std::uint8_t { Rectangle, Text, Image };

struct UiDrawCommand final {
    UiDrawCommandType type{UiDrawCommandType::Rectangle};
    UiRect rect{};
    UiRect clip{};
    UiColor color{};
    float corner_radius{0.0F};
    float font_size{0.0F};
    std::uint64_t image_id{0};
    std::string text;
};

// Owns a stable frame-local list of UI draw operations. The game/render adapter
// drains this list and maps it to GPU commands; this type performs no GPU work.
class UiCommandBuffer final : public IUiDrawSink {
public:
    explicit UiCommandBuffer(std::size_t maximum_commands=100000U) noexcept
        :maximum_commands_(maximum_commands) {}

    void clear() noexcept {
        commands_.clear();
        overflowed_=false;
    }

    void draw_rect(
        UiRect rect,UiRect clip,UiColor color,float corner_radius) override {
        UiDrawCommand command;
        command.type=UiDrawCommandType::Rectangle;
        command.rect=rect;
        command.clip=clip;
        command.color=color;
        command.corner_radius=corner_radius;
        append(std::move(command));
    }

    void draw_text(
        UiRect rect,UiRect clip,std::string_view text,
        float font_size,UiColor color) override {
        UiDrawCommand command;
        command.type=UiDrawCommandType::Text;
        command.rect=rect;
        command.clip=clip;
        command.color=color;
        command.font_size=font_size;
        command.text.assign(text);
        append(std::move(command));
    }

    void draw_image(
        UiRect rect,UiRect clip,std::uint64_t image_id,UiColor tint) override {
        UiDrawCommand command;
        command.type=UiDrawCommandType::Image;
        command.rect=rect;
        command.clip=clip;
        command.color=tint;
        command.image_id=image_id;
        append(std::move(command));
    }

    [[nodiscard]] const std::vector<UiDrawCommand>& commands() const noexcept {
        return commands_;
    }
    [[nodiscard]] std::vector<UiDrawCommand> take_commands() noexcept {
        overflowed_=false;
        return std::exchange(commands_,{});
    }
    [[nodiscard]] bool overflowed() const noexcept { return overflowed_; }
    [[nodiscard]] std::size_t maximum_commands() const noexcept {
        return maximum_commands_;
    }

private:
    void append(UiDrawCommand command) {
        if(commands_.size()>=maximum_commands_) {
            overflowed_=true;
            return;
        }
        commands_.push_back(std::move(command));
    }

    std::size_t maximum_commands_{100000U};
    bool overflowed_{false};
    std::vector<UiDrawCommand> commands_;
};

} // namespace dream::ui
