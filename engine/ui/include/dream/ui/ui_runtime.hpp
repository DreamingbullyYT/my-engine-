#pragma once
#include <dream/core/handle.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dream::ui {
struct UiNodeTag;
using UiNodeHandle=core::Handle<UiNodeTag>;
struct UiVec2 final { float x{0},y{0}; };
struct UiRect final {
    float x{0},y{0},width{0},height{0};
    [[nodiscard]] bool contains(float px,float py) const noexcept {
        return px>=x&&py>=y&&px<=x+width&&py<=y+height;
    }
    [[nodiscard]] bool empty() const noexcept { return width<=0||height<=0; }
};
struct UiColor final { float r{0},g{0},b{0},a{0}; };
[[nodiscard]] inline UiRect intersect(UiRect a,UiRect b) noexcept {
    const float l=std::max(a.x,b.x),t=std::max(a.y,b.y);
    const float r=std::min(a.x+a.width,b.x+b.width);
    const float d=std::min(a.y+a.height,b.y+b.height);
    return {l,t,std::max(0.0F,r-l),std::max(0.0F,d-t)};
}
enum class UiNodeType:std::uint8_t { Panel,Text,Image,Button,Toggle,Slider };
enum class LayoutDirection:std::uint8_t { Vertical,Horizontal };
enum class SizeMode:std::uint8_t { Fixed,Percent,Content,Fill };
enum class AlignItems:std::uint8_t { Start,Center,End,Stretch };
enum class JustifyContent:std::uint8_t { Start,Center,End,SpaceBetween };
struct UiSize final {
    SizeMode mode{SizeMode::Fill}; float value{1.0F};
    [[nodiscard]] static constexpr UiSize fixed(float v) noexcept {return {SizeMode::Fixed,v};}
    [[nodiscard]] static constexpr UiSize percent(float v) noexcept {return {SizeMode::Percent,v};}
    [[nodiscard]] static constexpr UiSize content() noexcept {return {SizeMode::Content,0};}
    [[nodiscard]] static constexpr UiSize fill(float w=1.0F) noexcept {return {SizeMode::Fill,w};}
};
struct UiStyle final {
    UiSize width{UiSize::fill()},height{UiSize::content()};
    LayoutDirection direction{LayoutDirection::Vertical};
    AlignItems align_items{AlignItems::Stretch};
    JustifyContent justify_content{JustifyContent::Start};
    float padding{0},gap{0},font_size{16},corner_radius{0};
    UiColor background{},foreground{1,1,1,1},accent{0.2F,0.55F,0.95F,1};
    bool visible{true},enabled{true},clip_children{true};
};
struct UiNodeDescription final {
    UiNodeType type{UiNodeType::Panel};
    std::string text;
    std::uint64_t image_id{0};
    UiStyle style{};
    UiVec2 intrinsic_size{};
    float value{0},minimum{0},maximum{1};
};
enum class UiError:std::uint8_t {
    None,InvalidNode,InvalidParent,InvalidStyle,InvalidValue,
    NotInteractive,CapacityExhausted,InvalidViewport
};
struct UiCreateResult final {
    UiNodeHandle handle{}; UiError error{UiError::None};
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle)&&error==UiError::None;
    }
};
struct UiNodeState final {
    UiNodeHandle handle{}; UiNodeType type{UiNodeType::Panel};
    UiRect rect{},clip{}; float value{0};
    bool enabled{false},visible{false};
};
class IUiTextMetrics {
public:
    virtual ~IUiTextMetrics()=default;
    [[nodiscard]] virtual UiVec2 measure(std::string_view,float) const noexcept=0;
};
enum class UiEventType:std::uint8_t { Pressed,Released,Clicked,ValueChanged,FocusChanged };
struct UiEvent final { UiEventType type{UiEventType::Clicked};UiNodeHandle target{};float value{0}; };
class IUiDrawSink {
public:
    virtual ~IUiDrawSink()=default;
    virtual void draw_rect(UiRect,UiRect,UiColor,float)=0;
    virtual void draw_text(UiRect,UiRect,std::string_view,float,UiColor)=0;
    virtual void draw_image(UiRect,UiRect,std::uint64_t,UiColor)=0;
};
enum class PointerAction:std::uint8_t { Down,Move,Up,Cancel };
struct UiPointerEvent final { PointerAction action{PointerAction::Move};float x{0},y{0}; };

class UiRuntime final {
public:
    static constexpr std::size_t maximum_nodes=10000U;
    UiRuntime()=default;
    UiRuntime(const UiRuntime&)=delete;
    UiRuntime& operator=(const UiRuntime&)=delete;

    [[nodiscard]] UiCreateResult create_node(UiNodeHandle parent,UiNodeDescription d) {
        if(!valid_description(d)) return {{},UiError::InvalidStyle};
        Node* parent_node=nullptr;
        if(parent) {
            parent_node=find_node(parent);
            if(!parent_node||parent_node->description.type!=UiNodeType::Panel)
                return {{},UiError::InvalidParent};
        }
        if(nodes_.size()>=maximum_nodes) return {{},UiError::CapacityExhausted};
        const auto h=handles_.allocate();
        if(!h) return {{},UiError::CapacityExhausted};
        Node n;n.handle=h;n.parent=parent;n.description=std::move(d);
        n.value=std::clamp(n.description.value,n.description.minimum,n.description.maximum);
        try {
            auto [it,ok]=nodes_.emplace(h.index(),std::move(n));
            (void)it;
            if(!ok){(void)handles_.release(h);return {{},UiError::CapacityExhausted};}
            if(parent_node) parent_node->children.push_back(h); else roots_.push_back(h);
        } catch(...) {nodes_.erase(h.index());(void)handles_.release(h);throw;}
        return {h,UiError::None};
    }
    [[nodiscard]] bool remove_node(UiNodeHandle h) {
        if(!find_node(h)) return false;remove_recursive(h);return true;
    }
    [[nodiscard]] bool contains(UiNodeHandle h) const noexcept {return find_node(h)!=nullptr;}
    [[nodiscard]] std::size_t node_count() const noexcept {return nodes_.size();}
    [[nodiscard]] UiNodeHandle focused_node() const noexcept {return focused_;}

    [[nodiscard]] UiError layout(UiRect viewport,const IUiTextMetrics& metrics) {
        if(viewport.empty()||!finite(viewport)) return UiError::InvalidViewport;
        viewport_=viewport;
        for(auto h:roots_) if(auto* n=find_node(h)) arrange(*n,viewport,viewport,metrics);
        layout_valid_=true;return UiError::None;
    }
    [[nodiscard]] std::optional<UiNodeState> state(UiNodeHandle h) const noexcept {
        const auto* n=find_node(h);if(!n)return std::nullopt;
        return UiNodeState{n->handle,n->description.type,n->rect,n->clip,n->value,
            n->description.style.enabled,n->description.style.visible};
    }
    [[nodiscard]] UiError pointer_event(const UiPointerEvent& e) {
        if(!layout_valid_||!std::isfinite(e.x)||!std::isfinite(e.y))
            return UiError::InvalidViewport;
        switch(e.action) {
        case PointerAction::Down: {
            const auto h=hit_test(e.x,e.y);
            if(!h){set_focus({});return UiError::None;}
            auto* n=find_node(h);if(!n)return UiError::None;
            set_focus(h);pointer_capture_=h;n->pressed=true;push_event(UiEventType::Pressed,*n);
            if(n->description.type==UiNodeType::Slider) update_slider(*n,e.x);
            return UiError::None;
        }
        case PointerAction::Move:
            if(auto* n=find_node(pointer_capture_))
                if(n->description.type==UiNodeType::Slider) update_slider(*n,e.x);
            return UiError::None;
        case PointerAction::Up: {
            auto* n=find_node(pointer_capture_);
            if(n) {
                const auto captured=n->handle;n->pressed=false;push_event(UiEventType::Released,*n);
                if(captured==hit_test(e.x,e.y)) activate(*n);
            }
            pointer_capture_={};return UiError::None;
        }
        case PointerAction::Cancel:
            if(auto* n=find_node(pointer_capture_)) n->pressed=false;
            pointer_capture_={};return UiError::None;
        }
        return UiError::None;
    }
    [[nodiscard]] bool focus(UiNodeHandle h) noexcept {
        if(h) {const auto* n=find_node(h);
            if(!n||!n->description.style.enabled||!interactive(n->description.type))return false;}
        set_focus(h);return true;
    }
    [[nodiscard]] bool activate_focused() {
        auto* n=find_node(focused_);if(!n||!n->description.style.enabled)return false;
        activate(*n);return true;
    }
    [[nodiscard]] bool pop_event(UiEvent& out) {
        if(events_.empty())return false;out=events_.front();events_.pop_front();return true;
    }
    void draw(IUiDrawSink& sink) const {
        if(!layout_valid_)return;
        for(auto h:roots_) if(const auto* n=find_node(h))draw_node(*n,sink);
    }

private:
    struct Node final {
        UiNodeHandle handle{},parent{};
        std::vector<UiNodeHandle> children;
        UiNodeDescription description;
        UiRect rect{},clip{};
        float value{0};
        bool pressed{false};
    };
    [[nodiscard]] static bool finite(UiVec2 v) noexcept {
        return std::isfinite(v.x)&&std::isfinite(v.y);
    }
    [[nodiscard]] static bool finite(UiRect r) noexcept {
        return std::isfinite(r.x)&&std::isfinite(r.y)&&std::isfinite(r.width)&&
            std::isfinite(r.height)&&r.width>=0&&r.height>=0;
    }
    [[nodiscard]] static bool finite(UiColor c) noexcept {
        return std::isfinite(c.r)&&std::isfinite(c.g)&&std::isfinite(c.b)&&
            std::isfinite(c.a)&&c.r>=0&&c.r<=1&&c.g>=0&&c.g<=1&&
            c.b>=0&&c.b<=1&&c.a>=0&&c.a<=1;
    }
    [[nodiscard]] static bool valid_size(UiSize s) noexcept {
        if(!std::isfinite(s.value)||s.value<0)return false;
        switch(s.mode) {
        case SizeMode::Fixed:case SizeMode::Content:return true;
        case SizeMode::Percent:return s.value<=10;
        case SizeMode::Fill:return s.value>0&&s.value<=1000;
        }
        return false;
    }
    [[nodiscard]] static bool valid_description(const UiNodeDescription& d) noexcept {
        const auto& s=d.style;
        switch(d.type) {
        case UiNodeType::Panel:case UiNodeType::Text:case UiNodeType::Image:
        case UiNodeType::Button:case UiNodeType::Toggle:case UiNodeType::Slider:break;
        default:return false;
        }
        switch(s.direction) {case LayoutDirection::Vertical:case LayoutDirection::Horizontal:break;default:return false;}
        switch(s.align_items) {case AlignItems::Start:case AlignItems::Center:case AlignItems::End:case AlignItems::Stretch:break;default:return false;}
        switch(s.justify_content) {case JustifyContent::Start:case JustifyContent::Center:case JustifyContent::End:case JustifyContent::SpaceBetween:break;default:return false;}
        if(!valid_size(s.width)||!valid_size(s.height)||!std::isfinite(s.padding)||s.padding<0||
           !std::isfinite(s.gap)||s.gap<0||!std::isfinite(s.font_size)||s.font_size<=0||
           !std::isfinite(s.corner_radius)||s.corner_radius<0||!finite(s.background)||
           !finite(s.foreground)||!finite(s.accent)||!finite(d.intrinsic_size)||
           d.intrinsic_size.x<0||d.intrinsic_size.y<0||d.text.size()>4096U||
           !std::isfinite(d.value))return false;
        if(d.type==UiNodeType::Slider&&(!std::isfinite(d.minimum)||
           !std::isfinite(d.maximum)||d.minimum>=d.maximum))return false;
        return true;
    }
    [[nodiscard]] static bool interactive(UiNodeType t) noexcept {
        return t==UiNodeType::Button||t==UiNodeType::Toggle||t==UiNodeType::Slider;
    }
    [[nodiscard]] Node* find_node(UiNodeHandle h) noexcept {
        if(!h)return nullptr;auto it=nodes_.find(h.index());
        return it!=nodes_.end()&&it->second.handle==h?&it->second:nullptr;
    }
    [[nodiscard]] const Node* find_node(UiNodeHandle h) const noexcept {
        if(!h)return nullptr;auto it=nodes_.find(h.index());
        return it!=nodes_.end()&&it->second.handle==h?&it->second:nullptr;
    }
    void remove_recursive(UiNodeHandle h) {
        auto* n=find_node(h);if(!n)return;const auto children=n->children;
        for(auto child:children)remove_recursive(child);
        n=find_node(h);if(!n)return;
        if(n->parent) {if(auto* p=find_node(n->parent)) {
            auto& v=p->children;v.erase(std::remove(v.begin(),v.end(),h),v.end());
        }} else roots_.erase(std::remove(roots_.begin(),roots_.end(),h),roots_.end());
        if(focused_==h)focused_={};if(pointer_capture_==h)pointer_capture_={};
        (void)handles_.release(h);nodes_.erase(h.index());
    }
    void set_focus(UiNodeHandle h) {
        if(focused_==h)return;focused_=h;
        if(auto* n=find_node(h))push_event(UiEventType::FocusChanged,*n);
        else events_.push_back({UiEventType::FocusChanged,{},0});
    }
    void push_event(UiEventType type,const Node& n) {
        events_.push_back({type,n.handle,n.value});
    }
    [[nodiscard]] UiVec2 measure(const Node& n,const IUiTextMetrics& metrics) const {
        const auto& d=n.description;
        switch(d.type) {
        case UiNodeType::Text:return metrics.measure(d.text,d.style.font_size);
        case UiNodeType::Image:return d.intrinsic_size;
        case UiNodeType::Button:case UiNodeType::Toggle: {
            const auto m=metrics.measure(d.text,d.style.font_size);
            const float icon=d.type==UiNodeType::Toggle?22.0F:0.0F;
            return {m.x+icon+2*d.style.padding,std::max(m.y,icon)+2*d.style.padding};
        }
        case UiNodeType::Slider:return {120+2*d.style.padding,24+2*d.style.padding};
        case UiNodeType::Panel: {
            float main=0,cross=0;std::size_t count=0;
            for(auto h:n.children)if(const auto* c=find_node(h)) {
                if(!c->description.style.visible)continue;
                const auto m=measure(*c,metrics);
                if(d.style.direction==LayoutDirection::Vertical){main+=m.y;cross=std::max(cross,m.x);}
                else {main+=m.x;cross=std::max(cross,m.y);}
                ++count;
            }
            if(count>1)main+=d.style.gap*static_cast<float>(count-1);
            main+=2*d.style.padding;cross+=2*d.style.padding;
            return d.style.direction==LayoutDirection::Vertical?UiVec2{cross,main}:UiVec2{main,cross};
        }}
        return {};
    }
    [[nodiscard]] static float resolve(UiSize s,float available,float content) noexcept {
        switch(s.mode){case SizeMode::Fixed:return s.value;case SizeMode::Percent:return available*s.value;
        case SizeMode::Content:return content;case SizeMode::Fill:return 0;}
        return 0;
    }
    void arrange(Node& n,UiRect rect,UiRect parent_clip,const IUiTextMetrics& metrics) {
        if(!n.description.style.visible){n.rect={};n.clip={};return;}
        n.rect=rect;n.clip=intersect(parent_clip,rect);
        if(n.description.type!=UiNodeType::Panel||n.children.empty())return;
        const auto& style=n.description.style;const float p=style.padding;
        const UiRect inner{rect.x+p,rect.y+p,std::max(0.0F,rect.width-2*p),std::max(0.0F,rect.height-2*p)};
        const bool vertical=style.direction==LayoutDirection::Vertical;
        const float main_available=vertical?inner.height:inner.width;
        const float cross_available=vertical?inner.width:inner.height;
        struct Child final {Node* node;float main{0},cross{0},weight{0};};
        std::vector<Child> children;children.reserve(n.children.size());
        float fixed=0,total_weight=0;
        for(auto h:n.children)if(auto* c=find_node(h)) {
            if(!c->description.style.visible)continue;
            const auto m=measure(*c,metrics);const auto& cs=c->description.style;
            const auto ms=vertical?cs.height:cs.width;const auto xs=vertical?cs.width:cs.height;
            Child entry{c};
            const float want_main=vertical?m.y:m.x,want_cross=vertical?m.x:m.y;
            if(ms.mode==SizeMode::Fill){entry.weight=ms.value;total_weight+=entry.weight;}
            else {entry.main=resolve(ms,main_available,want_main);fixed+=entry.main;}
            entry.cross=xs.mode==SizeMode::Fill?cross_available:resolve(xs,cross_available,want_cross);
            children.push_back(entry);
        }
        const float gaps=children.size()>1?style.gap*static_cast<float>(children.size()-1):0;
        const float free=std::max(0.0F,main_available-fixed-gaps);
        float used=fixed+gaps;
        for(auto& c:children)if(c.weight>0){c.main=total_weight>0?free*c.weight/total_weight:0;used+=c.main;}
        const float extra=std::max(0.0F,main_available-used);
        float cursor=vertical?inner.y:inner.x,actual_gap=style.gap;
        switch(style.justify_content) {
        case JustifyContent::Start:break;case JustifyContent::Center:cursor+=extra*0.5F;break;
        case JustifyContent::End:cursor+=extra;break;
        case JustifyContent::SpaceBetween:if(children.size()>1)actual_gap+=extra/static_cast<float>(children.size()-1);break;
        }
        const UiRect child_clip=style.clip_children?n.clip:parent_clip;
        for(auto& c:children) {
            float cross=c.cross;
            if(style.align_items==AlignItems::Stretch)cross=cross_available;
            else cross=std::min(cross,cross_available);
            float offset=0;
            if(style.align_items==AlignItems::Center)offset=(cross_available-cross)*0.5F;
            else if(style.align_items==AlignItems::End)offset=cross_available-cross;
            UiRect cr;
            if(vertical){cr={inner.x+offset,cursor,cross,c.main};cursor+=c.main+actual_gap;}
            else {cr={cursor,inner.y+offset,c.main,cross};cursor+=c.main+actual_gap;}
            arrange(*c.node,cr,child_clip,metrics);
        }
    }
    [[nodiscard]] UiNodeHandle hit_test(float x,float y) const noexcept {
        for(auto it=roots_.rbegin();it!=roots_.rend();++it)
            if(const auto* n=find_node(*it)){auto h=hit_test_node(*n,x,y);if(h)return h;}
        return {};
    }
    [[nodiscard]] UiNodeHandle hit_test_node(const Node& n,float x,float y) const noexcept {
        const auto& s=n.description.style;
        if(!s.visible||!s.enabled||!n.clip.contains(x,y))return {};
        for(auto it=n.children.rbegin();it!=n.children.rend();++it)
            if(const auto* c=find_node(*it)){auto h=hit_test_node(*c,x,y);if(h)return h;}
        return interactive(n.description.type)&&n.rect.contains(x,y)?n.handle:UiNodeHandle{};
    }
    void update_slider(Node& n,float x) {
        const float f=std::clamp((x-n.rect.x)/std::max(1.0F,n.rect.width),0.0F,1.0F);
        const float v=n.description.minimum+(n.description.maximum-n.description.minimum)*f;
        if(v==n.value)return;n.value=v;push_event(UiEventType::ValueChanged,n);
    }
    void activate(Node& n) {
        if(n.description.type==UiNodeType::Button)push_event(UiEventType::Clicked,n);
        else if(n.description.type==UiNodeType::Toggle) {
            n.value=n.value>=0.5F?0.0F:1.0F;push_event(UiEventType::ValueChanged,n);
        }
    }
    void draw_node(const Node& n,IUiDrawSink& sink) const {
        if(!n.description.style.visible||n.clip.empty())return;
        const auto& s=n.description.style;
        if(s.background.a>0)sink.draw_rect(n.rect,n.clip,s.background,s.corner_radius);
        switch(n.description.type) {
        case UiNodeType::Text:case UiNodeType::Button:
            sink.draw_text(n.rect,n.clip,n.description.text,s.font_size,s.foreground);break;
        case UiNodeType::Toggle: {
            const float side=std::min(18.0F,n.rect.height);
            UiRect box{n.rect.x+s.padding,n.rect.y+(n.rect.height-side)*0.5F,side,side};
            sink.draw_rect(box,n.clip,n.value>=0.5F?s.accent:s.foreground,2);
            UiRect text{box.x+side+6,n.rect.y,std::max(0.0F,n.rect.width-side-6),n.rect.height};
            sink.draw_text(text,n.clip,n.description.text,s.font_size,s.foreground);break;
        }
        case UiNodeType::Image:if(n.description.image_id)sink.draw_image(n.rect,n.clip,n.description.image_id,s.foreground);break;
        case UiNodeType::Slider: {
            const float cy=n.rect.y+n.rect.height*0.5F;
            sink.draw_rect({n.rect.x,cy-2,n.rect.width,4},n.clip,s.foreground,2);
            const float f=(n.value-n.description.minimum)/(n.description.maximum-n.description.minimum);
            sink.draw_rect({n.rect.x+std::clamp(f,0.0F,1.0F)*n.rect.width-7,cy-7,14,14},n.clip,s.accent,7);break;
        }
        case UiNodeType::Panel:break;
        }
        for(auto h:n.children)if(const auto* c=find_node(h))draw_node(*c,sink);
    }

    core::HandlePool<UiNodeTag> handles_;
    std::map<std::uint32_t,Node> nodes_;
    std::vector<UiNodeHandle> roots_;
    std::deque<UiEvent> events_;
    UiNodeHandle focused_{},pointer_capture_{};
    UiRect viewport_{};
    bool layout_valid_{false};
};
} // namespace dream::ui
