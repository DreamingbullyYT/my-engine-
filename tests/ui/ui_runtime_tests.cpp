#include <dream/ui/ui_runtime.hpp>
#include <cstdlib>
#include <iostream>
#include <string_view>
namespace {
using namespace dream::ui;
void require(bool ok,std::string_view msg){if(!ok){std::cerr<<"FAILED: "<<msg<<'\n';std::exit(EXIT_FAILURE);}}
class Metrics final:public IUiTextMetrics{
public:UiVec2 measure(std::string_view s,float size)const noexcept override{return {static_cast<float>(s.size())*size*0.5F,size*1.25F};}
};
class Sink final:public IUiDrawSink{
public:std::size_t rects=0,texts=0,images=0;
void draw_rect(UiRect,UiRect,UiColor,float)override{++rects;}
void draw_text(UiRect,UiRect,std::string_view,float,UiColor)override{++texts;}
void draw_image(UiRect,UiRect,std::uint64_t,UiColor)override{++images;}
};
}
int main(){
    UiRuntime ui;
    UiNodeDescription root_d;root_d.type=UiNodeType::Panel;
    root_d.style.width=UiSize::fill();root_d.style.height=UiSize::fill();
    root_d.style.padding=10;root_d.style.gap=8;
    root_d.style.background={0.05F,0.05F,0.08F,1};
    const auto root=ui.create_node({},root_d);
    require(static_cast<bool>(root),"root panel creates");
    UiNodeDescription button_d;button_d.type=UiNodeType::Button;
    button_d.text="Start game";button_d.style.width=UiSize::fill();
    button_d.style.height=UiSize::fixed(40);button_d.style.background={0.1F,0.2F,0.3F,1};
    const auto button=ui.create_node(root.handle,button_d);
    require(static_cast<bool>(button),"button creates");
    UiNodeDescription toggle_d;toggle_d.type=UiNodeType::Toggle;toggle_d.text="Fullscreen";
    toggle_d.style.width=UiSize::fill();toggle_d.style.height=UiSize::fixed(32);
    const auto toggle=ui.create_node(root.handle,toggle_d);
    require(static_cast<bool>(toggle),"toggle creates");
    UiNodeDescription slider_d;slider_d.type=UiNodeType::Slider;
    slider_d.minimum=0;slider_d.maximum=100;slider_d.value=25;
    slider_d.style.width=UiSize::fill();slider_d.style.height=UiSize::fixed(28);
    const auto slider=ui.create_node(root.handle,slider_d);
    require(static_cast<bool>(slider),"slider creates");
    UiNodeDescription text_d;text_d.type=UiNodeType::Text;text_d.text="Settings";
    text_d.style.width=UiSize::content();text_d.style.height=UiSize::content();
    const auto label=ui.create_node(root.handle,text_d);
    require(static_cast<bool>(label),"text creates");

    const Metrics metrics;
    require(ui.layout({0,0,320,240},metrics)==UiError::None,"layout works");
    const auto bs=ui.state(button.handle),ss=ui.state(slider.handle);
    require(bs&&bs->rect.width==300&&bs->rect.height==40,
        "vertical flex layout fills button width and keeps fixed height");
    require(ss&&ss->rect.y>bs->rect.y,"children are arranged in order");

    ui.pointer_event({PointerAction::Down,bs->rect.x+10,bs->rect.y+10});
    ui.pointer_event({PointerAction::Up,bs->rect.x+10,bs->rect.y+10});
    UiEvent ev;bool clicked=false;
    while(ui.pop_event(ev))if(ev.type==UiEventType::Clicked&&ev.target==button.handle)clicked=true;
    require(clicked,"button emits Clicked event");

    const auto ts=ui.state(toggle.handle);
    ui.pointer_event({PointerAction::Down,ts->rect.x+8,ts->rect.y+8});
    ui.pointer_event({PointerAction::Up,ts->rect.x+8,ts->rect.y+8});
    require(ui.state(toggle.handle)->value==1,"toggle changes boolean value");

    const auto sl=ui.state(slider.handle);
    ui.pointer_event({PointerAction::Down,sl->rect.x+sl->rect.width,sl->rect.y+sl->rect.height*0.5F});
    require(ui.state(slider.handle)->value==100,"slider maps pointer to its value range");
    ui.pointer_event({PointerAction::Up,sl->rect.x+sl->rect.width,sl->rect.y+sl->rect.height*0.5F});

    Sink sink;ui.draw(sink);
    require(sink.rects>=2&&sink.texts>=2,"draw sink receives visual commands");
    UiNodeDescription bad;bad.type=UiNodeType::Slider;bad.minimum=1;bad.maximum=1;
    require(!static_cast<bool>(ui.create_node({},bad)),"invalid slider bounds reject");
    require(ui.create_node(button.handle,UiNodeDescription{.type=UiNodeType::Button}).error==UiError::InvalidParent,
        "only panels accept children");
    require(ui.remove_node(root.handle)&&ui.node_count()==0,"root removal removes subtree");
    require(!ui.contains(button.handle)&&!ui.focused_node(),"stale widget references clear");
    require(ui.layout({0,0,0,0},metrics)==UiError::InvalidViewport,"invalid viewport rejects");
    return EXIT_SUCCESS;
}
