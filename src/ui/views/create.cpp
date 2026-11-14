#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::create() {
    shell("cover");
    heading("NEW CAMPAIGN","Choose your operative","Six specialists. Each has a different suit, mobility and supply loadout.");
    for (int i=0;i<static_cast<int>(characters.size());++i) {
        const auto& c=characters[i];
        const Rect card{54.f+(i%3)*367.f,166.f+(i/3)*153.f,355,143};
        const bool chosen=i==newCharacter;
        r.rect(card,hex(0x0b161e).alpha(.94f));
        r.cover(c.art,{card.x+1,card.y+1,127,card.h-2},1,{.5f,.26f});
        r.gradient({card.x+91,card.y,63,card.h},hex(0x0b151e).alpha(0),hex(0x0b151e));
        r.rect({card.x+130,card.y,card.w-130,card.h},hex(0x0a121b).alpha(.53f));
        const Color tint=hex(c.color);
        r.rect({card.x+130,card.y+10,3,card.h-20},tint.alpha(.65f));
        label({card.x+145,card.y+28},c.role,tint);
        r.text({card.x+145,card.y+64},c.name,23,white,true);
        r.text({card.x+145,card.y+94},c.trait,12,pale);
        r.text({card.x+145,card.y+121},chosen?"SELECTED  /  READY":"SELECT OPERATIVE",11,chosen?tint:muted,true);
        r.outline(card,chosen?tint:hovered(card)?white:hex(0x456070),chosen?2.5f:1.2f);
        hit(card,[this,i]{newCharacter=i; audio.event("switch",.6f);},true);
    }
    label({54,483},"DIFFICULTY");
    label({493,483},"SAVE SLOT");
    for (int i=0;i<3;++i) {
        button({54.f+i*135,495,125,35},difficulties[i].name,[this,i]{newDifficulty=i;},true,i==newDifficulty);
        button({493.f+i*101,495,94,35},"Slot "+std::to_string(i+1)+(slots[i]?" *":""),[this,i]{newSlot=i+1;},true,newSlot==i+1);
    }
    button({54,549,119,38},"BACK",[this]{show(View::menu);});
    button({886,549,260,38},"START CAMPAIGN",[this] {
        if (!slots[newSlot-1]) beginCampaign();
        else ask(View::create,"Replace slot "+std::to_string(newSlot)+"?",
                 "Starting a new campaign will replace the progress in this slot.",[this]{beginCampaign();});
    },true,true);
}
} // namespace vl
