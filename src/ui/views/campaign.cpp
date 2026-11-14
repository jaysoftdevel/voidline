#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::campaign() {
    const auto& stage=stages[sector];
    shell(stage.art); heading("CAMPAIGN","Sector map",
        std::string(characters[save.character].name)+"  /  "+difficulties[save.difficulty].name+
        "  /  "+std::to_string(save.score)+" lifetime score");
    r.cover(stage.art,{55,171,598,201},1.02f);
    r.gradient({55,171,598,201},hex(0x101920).alpha(.08f),hex(0x09121e).alpha(.79f));
    r.outline({55,171,598,201},hex(stage.accent).alpha(.69f));
    label({70,317},"SECTOR "+digits(sector+1)+"   /   "+stage.threat+
        "   /   ROUTE "+std::to_string(int(stage.worldWidth)),hex(stage.accent));
    r.text({69,354},stage.name,39,white,true);
    r.gradient({671,171,474,201},hex(0x182c39),hex(0x101d29));
    r.outline({671,171,474,201},hex(0x465969));
    label({687,197},stage.location,teal);
    copy({687,225},stage.briefing,419);
    label({687,274},std::string("GUARDIAN  /  ")+stage.boss,amber);
    const auto& best=save.records[sector];
    if (best.time>0) label({687,297},"BEST  /  "+std::to_string(best.score)+" SCORE  /  "+timeText(best.time));
    button({687,316,442,40},sector<=save.cleared?"DEPLOY":"CLEAR PREVIOUS SECTOR",[this]{launch(sector);},sector<=save.cleared,true);
    constexpr int perPage=5;
    const int first=(sector/perPage)*perPage;
    const int end=std::min(first+perPage,sectorCount);
    for (int i=first;i<end;++i) {
        const auto& tile=stages[i];
        const Rect box{55.f+221*(i-first),392,206,112};
        r.cover(tile.art,box,1,{.5f,.52f},hex(i>save.cleared?0x75818a:0xc7d7dd));
        r.gradient(box,hex(0x0a141d).alpha(.23f),hex(0x0a141d).alpha(.92f));
        r.outline(box,i==sector?hex(stage.accent):hovered(box)?white:hex(0x4c606d),i==sector?2:1);
        label({box.x+10,box.y+22},"SECTOR "+digits(i+1),pale);
        r.text({box.x+10,box.y+75},tile.name,20,white,true);
        label({box.x+10,box.y+96},i<save.cleared?"CLEARED":i==save.cleared?"AVAILABLE":"LOCKED",i<save.cleared?teal:pale);
        hit(box,[this,i]{sector=i;},true);
    }
    button({55,528,148,40},"MAIN MENU",[this]{show(View::menu);});
    button({365,528,82,40},"PREV",[this,first,perPage]{sector=first-perPage;},first>0);
    label({461,552},"SECTORS "+digits(first+1)+"-"+digits(end)+" / "+digits(sectorCount),pale);
    button({612,528,82,40},"NEXT",[this,end]{sector=end;},end<sectorCount);
    button({972,528,173,40},"ARMORY",[this]{showArmory();},true,true);
    label({737,552},std::to_string(save.credits)+" UPGRADE POINTS",teal);
}
} // namespace vl
