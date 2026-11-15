#include "ui/app.hpp"
#include "ui/common.hpp"
#include <iomanip>
#include <sstream>

using namespace vl::ui;

namespace vl {
void App::armory() {
    shell("foundry");
    const bool debug=settings.debugUnlockLevels;
    const Save& loadout=effectiveLoadout();
    heading("EQUIPMENT","Armory",debug?"DEBUG LOADOUT  /  upgrades cycle 0 -> max -> 0; no credits used":
        std::to_string(loadout.credits)+" credits available");
    constexpr int operative=static_cast<int>(weaponCount);
    constexpr int skillTab=operative+1;
    constexpr float tabGap=5,tabWidth=(1090-tabGap*(weaponCount+1))/(weaponCount+2);
    for (int i=0;i<=skillTab;++i) {
        const int id=i<operative?weaponDisplayOrder[i]:i;
        const bool locked=!debug && i<operative && loadout.cleared<weapons[id].unlock;
        const std::string name=i==operative?"OPERATIVE":i==skillTab?"SKILLS":
            std::string(weapons[id].shortName)+(locked?" [L]":!loadout.owned[id]?" [$]":"");
        const Rect box{55+i*(tabWidth+tabGap),162,tabWidth,34};
        button(box,name,[this,id]{weapon=id;},true,id==weapon);
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
        const auto& t=loadout.tiers[weapon];
        const Color tint=hex(w.color);
        const bool unlocked=debug || loadout.cleared>=w.unlock;
        const bool owned=loadout.owned[weapon];
        r.gradient({55,210,430,300},hex(0x1a2c37),hex(0x101b26));
        r.outline({55,210,430,300},hex(0x485b68));
        r.gradient({56,211,428,150},hex(0x0d1720).alpha(.55f),hex(0x0d1720).alpha(.12f));
        r.showcaseGun(weapon,{70,216,400,140});
        if (!owned) r.rect({56,211,428,150},hex(0x070b12).alpha(.60f));
        label({69,383},w.type,tint);
        r.text({69,410},w.name,24,white,true);
        copy({69,432},w.description,400,13.5f,pale);
        const float rate=1/(w.interval*(1-t[5]*.08f));
        stat(69,470,"DAMAGE",std::to_string(int(std::lround(w.damage*(1+t[0]*.2f))))+(w.pellets>1?" x"+std::to_string(w.pellets):""),white);
        stat(247,470,"RANGE",std::to_string(int(std::lround(w.range*(1+t[4]*.14f)))),white);
        std::ostringstream rateText; rateText<<std::fixed<<std::setprecision(1)<<rate<<"/s";
        stat(69,492,"RATE",rateText.str(),white);
        stat(247,492,"MAGAZINE",std::to_string(int(std::lround(w.magazine*(1+t[1]*.25f)))),white);
        if (!owned) {
            button({55,522,290,38},!unlocked?"CLEAR SECTOR "+digits(w.unlock):"BUY WEAPON  "+std::to_string(weaponPrices[weapon]),
                   [this]{buyWeapon();},unlocked && loadout.credits>=weaponPrices[weapon],true);
        } else {
            button({55,522,216,38},weapon==loadout.equipped?"EQUIPPED":"EQUIP WEAPON",
                   [this]{equip();},weapon!=loadout.equipped,true);
        }
        for (int i=0;i<static_cast<int>(upgradeCount);++i) {
            const Rect cell{500.f+(i%2)*329,207.f+(i/2)*61,316,56};
            const int tier=t[i], price=upgradePrice(i);
            const char* title=i<8?upgrades[i].name:weaponSpecials[weapon][i-8].name;
            const char* detail=i<8?upgrades[i].description:weaponSpecials[weapon][i-8].description;
            r.rect(cell,hex(0x142632));
            r.outline(cell,hex(0x465c6b));
            r.text({cell.x+11,cell.y+19},title,14,white,true);
            copy({cell.x+11,cell.y+34},detail,196,10.5f,pale);
            pips({cell.x,cell.y-7,cell.w,cell.h},3,tier,tint);
            button({cell.right()-104,cell.y+6,94,29},debug?(tier>=3?"RESET":"RANK +1"):
                   tier>=3?"MAX":"BUY  "+std::to_string(price),[this,i]{buy(i);},
                   debug || (owned && tier<3 && loadout.credits>=price));
        }
    } else if (weapon==operative) {
        const auto& c=characters[loadout.character];
        const Color tint=hex(c.color);
        const auto& rank=loadout.perks;
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
        line(347,"SUIT",std::to_string(int(std::lround(c.health*(1+.10f*rank[0]))))+" integrity");
        line(366,"SPEED",std::to_string(100+4*rank[1])+"%");
        line(385,"JUMP",std::to_string(100+3*rank[1])+"%");
        std::ostringstream reserve; reserve<<std::fixed<<std::setprecision(1)<<6.25f*(1+.25f*rank[2])<<" s";
        line(404,"BULLET TIME",reserve.str());
        line(423,"AMMO PICKUP",std::to_string(int(std::lround(41*(1+.2f*rank[3]))))+"% of max");
        line(442,"REPAIR",std::to_string(int(std::lround(42*(1+.15f*rank[4]))))+" suit");
        line(461,"DAMAGE TAKEN",std::to_string(100-6*rank[5])+"%");
        line(480,"LIVES",std::to_string(startingLives(loadout.difficulty,loadout.extraLives))+" per mission");
        for (int i=0;i<static_cast<int>(perkCount);++i) {
            const auto& p=perks[i];
            const Rect cell{500.f+(i%2)*329,210.f+(i/2)*76,316,68};
            const int level=rank[i], price=p.base+level*30;
            r.rect(cell,hex(0x142632));
            r.outline(cell,hex(0x465c6b));
            r.text({cell.x+11,cell.y+22},p.name,16,white,true);
            copy({cell.x+11,cell.y+39},p.description,196,11.5f,pale);
            pips(cell,p.maxRank,level,tint);
            button({cell.right()-104,cell.y+10,94,32},debug?(level>=p.maxRank?"RESET":"RANK +1"):
                   level>=p.maxRank?"MAX":"BUY  "+std::to_string(price),[this,i]{buyPerk(i);},
                   debug || (level<p.maxRank && loadout.credits>=price));
        }
        const int cost=extraLifePrice(loadout.extraLives);
        r.rect({500,450,645,68},hex(0x142632));
        r.outline({500,450,645,68},hex(0x465c6b));
        r.text({513,472},"EXTRA LIVES",16,tint,true);
        r.text({513,490},"+1 starting life per mission ("+std::to_string(loadout.extraLives)+" / "+std::to_string(maxExtraLives)+" purchased)",12,pale);
        button({987,467,146,37},debug?(loadout.extraLives>=maxExtraLives?"RESET":"RANK +1"):
               loadout.extraLives>=maxExtraLives?"MAX":"BUY  "+std::to_string(cost),
               [this]{buyExtraLife();},debug || (loadout.extraLives<maxExtraLives && loadout.credits>=cost));
    }
    if (weapon==skillTab) {
        const int c=loadout.character;
        const auto& character=characters[c];
        const Color tint=hex(character.color);
        r.gradient({55,210,1090,300},hex(0x152a38),hex(0x101b26));
        r.cover(character.art,{65,220,145,275},1,{.5f,.25f});
        label({234,244},"CHARACTER ABILITIES",tint);
        r.text({234,283},character.name,26,white,true);
        copy({234,307},"Two personal skills, independent of shared operative perks. Each rank grants a mechanical benefit in every mission.",790,15,pale);
        for (int i=0;i<2;++i) {
            const auto& ability=characterSkills[c][i];
            const int rank=loadout.skills[c][i],cost=ability.base+rank*48;
            const Rect box{234.f+i*454,346,437,140};
            r.rect(box,hex(0x132536)); r.outline(box,hex(0x49677a));
            r.text({box.x+18,box.y+32},ability.name,20,tint,true);
            copy({box.x+18,box.y+57},ability.description,385,13,pale);
            r.text({box.x+18,box.y+99},"RANK "+std::to_string(rank)+" / "+std::to_string(ability.maxRank),14,white,true);
            button({box.right()-157,box.bottom()-42,145,32},debug?(rank>=ability.maxRank?"RESET":"RANK +1"):
                   rank>=ability.maxRank?"MAX": "UPGRADE "+std::to_string(cost),
                   [this,i]{buySkill(i);},debug || (rank<ability.maxRank && loadout.credits>=cost));
        }
    }
    button({972,528,173,39},"CAMPAIGN",[this]{showMap();});
}
} // namespace vl
