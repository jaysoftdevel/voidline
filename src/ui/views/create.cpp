#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::create() {
    shell("cover");
    heading("NEW CAMPAIGN","Select an operative","Choose a suit, difficulty and save slot.");
    for (int i=0;i<3;++i) {
        const auto& c=characters[i];
        const Rect card{54.f+i*367,177,355,263};
        const bool chosen=i==newCharacter;
        r.cover(c.art,card,1,{.51f,.25f});
        r.gradient(card,hex(0x0b151c).alpha(.04f),hex(0x080e16).alpha(.94f));
        r.rect({card.x,card.bottom()-85,card.w,85},hex(0x101a23).alpha(.75f));
        r.outline(card,chosen?hex(c.color):hovered(card)?white:hex(0x4c5e6a),chosen?2.4f:1.25f);
        label({card.x+13,card.bottom()-60},c.role,hex(c.color));
        r.text({card.x+13,card.bottom()-35},c.name,24,white,true);
        r.text({card.x+13,card.bottom()-13},c.trait,14,pale);
        hit(card,[this,i]{newCharacter=i; audio.event("switch",.6f);},true);
    }
    label({54,463},"DIFFICULTY");
    label({493,463},"SAVE SLOT");
    for (int i=0;i<3;++i) {
        button({54.f+i*135,472,125,36},difficulties[i].name,[this,i]{newDifficulty=i;},true,i==newDifficulty);
        button({493.f+i*101,472,94,36},"Slot "+std::to_string(i+1)+(slots[i]?" *":""),[this,i]{newSlot=i+1;},true,newSlot==i+1);
    }
    button({54,532,119,38},"BACK",[this]{show(View::menu);});
    button({886,532,260,38},"START CAMPAIGN",[this] {
        if (!slots[newSlot-1]) beginCampaign();
        else ask(View::create,"Replace slot "+std::to_string(newSlot)+"?",
                 "Starting a new campaign will replace the progress in this slot.",[this]{beginCampaign();});
    },true,true);
}
} // namespace vl
