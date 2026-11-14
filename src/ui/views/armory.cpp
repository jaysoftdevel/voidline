#include "ui/app.hpp"
#include "ui/common.hpp"
#include <iomanip>
#include <sstream>

using namespace vl::ui;

namespace vl {
void App::armory() {
    shell("foundry");
    heading("EQUIPMENT","Armory",std::to_string(save.credits)+" upgrade points available");
    constexpr int operative=static_cast<int>(weaponCount);
    constexpr float tabGap=6,tabWidth=(1090-tabGap*weaponCount)/(weaponCount+1);
    for (int i=0;i<=operative;++i) {
        const bool locked=i<operative && save.cleared<weapons[i].unlock;
        const std::string name=i==operative?"OPERATIVE":std::string(weapons[i].shortName)+(locked?" (locked)":"");
        const Rect box{55+i*(tabWidth+tabGap),162,tabWidth,34};
        button(box,name,[this,i]{weapon=i;},true,i==weapon);
    }
    auto pips=[&](Rect cell,int count,int filled,Color tint) {
        const float span=count*13.f-5;
        for (int j=0;j<count;++j) r.rect({cell.right()-57-span*.5f+j*13.f,cell.y+52,8,8},j<filled?tint:hex(0x405160));
    };
    auto stat=[&](float x,float y,std::string_view name,const std::string& value,Color tint) {
        label({x,y},name,muted);
        r.text({x+68,y},value,14,tint,true);
    };
    if (weapon<operative) {
        const auto& w=weapons[weapon];
        const auto& t=save.tiers[weapon];
        const Color tint=hex(w.color);
        const bool unlocked=save.cleared>=w.unlock;
        r.gradient({55,210,430,300},hex(0x1a2c37),hex(0x101b26));
        r.outline({55,210,430,300},hex(0x485b68));
        r.gradient({56,211,428,150},hex(0x0d1720).alpha(.55f),hex(0x0d1720).alpha(.12f));
        r.showcaseGun(weapon,{70,216,400,140});
        if (!unlocked) r.rect({56,211,428,150},hex(0x070b12).alpha(.55f));
        label({69,383},w.type,tint);
        r.text({69,410},w.name,24,white,true);
        copy({69,432},w.description,400,13.5f,pale);
        const float rate=1/(w.interval*(1-t[5]*.08f));
        stat(69,470,"DAMAGE",std::to_string(int(std::lround(w.damage*(1+t[0]*.2f))))+(w.pellets>1?" x"+std::to_string(w.pellets):""),white);
        stat(247,470,"RANGE",std::to_string(int(std::lround(w.range*(1+t[4]*.14f)))),white);
        std::ostringstream rateText; rateText<<std::fixed<<std::setprecision(1)<<rate<<"/s";
        stat(69,492,"RATE",rateText.str(),white);
        stat(247,492,"MAGAZINE",std::to_string(int(std::lround(w.magazine*(1+t[1]*.25f)))),white);
        button({55,522,216,38},!unlocked?"CLEAR SECTOR "+digits(w.unlock):weapon==save.equipped?"EQUIPPED":"EQUIP WEAPON",
               [this]{equip();},unlocked && weapon!=save.equipped,true);
        for (int i=0;i<static_cast<int>(upgradeCount);++i) {
            const auto& u=upgrades[i];
            const Rect cell{500.f+(i%2)*329,210.f+(i/2)*76,316,68};
            const int tier=t[i], price=upgradePrice(i);
            r.rect(cell,hex(0x142632));
            r.outline(cell,hex(0x465c6b));
            r.text({cell.x+11,cell.y+22},u.name,16,white,true);
            copy({cell.x+11,cell.y+39},u.description,196,11.5f,pale);
            pips(cell,3,tier,tint);
            button({cell.right()-104,cell.y+10,94,32},tier>=3?"MAX":"BUY  "+std::to_string(price),[this,i]{buy(i);},
                   unlocked && tier<3 && save.credits>=price);
        }
    } else {
        const auto& c=characters[save.character];
        const Color tint=hex(c.color);
        const auto& rank=save.perks;
        r.gradient({55,210,430,300},hex(0x1a2c37),hex(0x101b26));
        r.outline({55,210,430,300},hex(0x485b68));
        r.cover(c.art,{56,211,150,298},1,{.5f,.2f});
        r.gradient({56,211,150,298},hex(0x0d1720).alpha(.05f),hex(0x0d1720).alpha(.55f));
        label({222,236},c.role,tint);
        r.text({222,265},c.name,25,white,true);
        copy({222,288},c.description,250,13.f,pale);
        auto line=[&](float y,std::string_view name,const std::string& value) {
            label({222,y},name,muted);
            r.text({372,y},value,14,white,true);
        };
        line(352,"SUIT",std::to_string(int(std::lround(c.health*(1+.10f*rank[0]))))+" integrity");
        line(374,"SPEED",std::to_string(100+4*rank[1])+"%");
        line(396,"JUMP",std::to_string(100+3*rank[1])+"%");
        std::ostringstream reserve; reserve<<std::fixed<<std::setprecision(1)<<6.25f*(1+.25f*rank[2])<<" s";
        line(418,"BULLET TIME",reserve.str());
        line(440,"AMMO PICKUP",std::to_string(int(std::lround(41*(1+.2f*rank[3]))))+"% of max");
        line(462,"REPAIR",std::to_string(int(std::lround(42*(1+.15f*rank[4]))))+" suit");
        line(484,"DAMAGE TAKEN",std::to_string(100-6*rank[5])+"%");
        for (int i=0;i<static_cast<int>(perkCount);++i) {
            const auto& p=perks[i];
            const Rect cell{500.f+(i%2)*329,210.f+(i/2)*76,316,68};
            const int level=rank[i], price=p.base+level*30;
            r.rect(cell,hex(0x142632));
            r.outline(cell,hex(0x465c6b));
            r.text({cell.x+11,cell.y+22},p.name,16,white,true);
            copy({cell.x+11,cell.y+39},p.description,196,11.5f,pale);
            pips(cell,p.maxRank,level,tint);
            button({cell.right()-104,cell.y+10,94,32},level>=p.maxRank?"MAX":"BUY  "+std::to_string(price),[this,i]{buyPerk(i);},
                   level<p.maxRank && save.credits>=price);
        }
        copy({500,452},"Operative upgrades apply to every weapon and carry through the whole campaign.",640,13.f,muted);
    }
    button({972,528,173,39},"CAMPAIGN",[this]{showMap();});
}
} // namespace vl
