#include "ui/app.hpp"
#include "ui/common.hpp"
#include <sstream>

using namespace vl::ui;

namespace vl {
bool App::hovered(Rect box) const {
    return box.contains(mouse) || (focus>=0 && static_cast<int>(buttons.size())==focus);
}

void App::hit(Rect box,std::function<void()> action,bool enabled) {
    if (enabled) buttons.push_back({box,std::move(action)});
}

void App::button(Rect b,std::string text,std::function<void()> action,bool enabled,bool primary) {
    const bool hover=enabled && hovered(b);
    const Color color=!enabled?hex(0x202a35):primary?(hover?hex(0xffba87):amber):(hover?hex(0x304550):hex(0x172831));
    r.gradient(b,color,color.energy(.85f));
    r.outline(b,hover||primary?amber.alpha(enabled?.89f:.3f):hex(0x3b5262),1.1f);
    const Color ink=!enabled?muted:primary?hex(0x181318):white;
    r.text({b.x+14,b.y+b.h*.5f+5},text,15,ink,true);
    hit(b,std::move(action),enabled);
}

void App::label(Vec at,std::string_view text,Color tint) { r.text(at,text,12,tint,true); }

void App::copy(Vec at,std::string_view text,float maxWidth,float size,Color tint) {
    std::istringstream words{std::string(text)};
    std::string row,word;
    while (words>>word) {
        const std::string combined=row.empty()?word:row+" "+word;
        if (!row.empty() && r.measure(combined,size)>maxWidth) {
            r.text(at,row,size,tint); at.y+=size*1.38f; row=word;
        } else row=combined;
    }
    if (!row.empty()) r.text(at,row,size,tint);
}

void App::shell(std::string_view art) {
    r.cover(art,screen,1.07f,{.58f,.51f},hex(0xc2cfe0));
    r.gradient(screen,hex(0x07111b).alpha(.73f),hex(0x09131e).alpha(.95f));
    r.rect({0,0,width,43},hex(0x0c1721).alpha(.97f));
    r.line({0,43},{width,43},1,hex(0x54616e).alpha(.60f));
    r.image("voidline-logo.png",{37,5,32,32});
    r.text({77,28},"VOIDLINE",18,white,true);
    label({1036,27},"FIELD OPERATIONS",muted);
}

void App::heading(std::string_view tag,std::string_view title,std::string_view detail) {
    label({55,78},tag,amber);
    r.text({53,123},title,48,white,true);
    if (!detail.empty()) r.text({57,151},detail,16,pale);
}
} // namespace vl
