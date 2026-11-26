#include <dream/ui/ui_command_buffer.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
}

int main() {
    dream::ui::UiCommandBuffer buffer(3U);
    const dream::ui::UiRect rect{10,20,30,40};
    const dream::ui::UiRect clip{0,0,100,100};
    buffer.draw_rect(rect,clip,{1,0,0,1},4.0F);
    buffer.draw_text(rect,clip,"Launch",18.0F,{1,1,1,1});
    buffer.draw_image(rect,clip,42U,{0.5F,0.5F,0.5F,1});
    require(buffer.commands().size()==3U&&!buffer.overflowed(),
        "command buffer preserves the three draw operations");
    require(buffer.commands()[0].type==dream::ui::UiDrawCommandType::Rectangle&&
            buffer.commands()[0].corner_radius==4.0F,
        "rectangle command stores geometry, clip and radius");
    require(buffer.commands()[1].type==dream::ui::UiDrawCommandType::Text&&
            buffer.commands()[1].text=="Launch"&&
            buffer.commands()[1].font_size==18.0F,
        "text command owns the string and font size");
    require(buffer.commands()[2].type==dream::ui::UiDrawCommandType::Image&&
            buffer.commands()[2].image_id==42U,
        "image command stores the engine asset identifier");
    buffer.draw_rect(rect,clip,{0,1,0,1},0);
    require(buffer.overflowed()&&buffer.commands().size()==3U,
        "buffer refuses to exceed its command capacity");
    const auto commands=buffer.take_commands();
    require(commands.size()==3U&&buffer.commands().empty()&&!buffer.overflowed(),
        "taking commands transfers ownership and resets overflow state");
    buffer.clear();
    require(buffer.commands().empty()&&!buffer.overflowed(),
        "clear resets the UI frame buffer");
    return EXIT_SUCCESS;
}
