#include "ui/app.hpp"
#include "game/weapon_selection.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"
#include "ui/level_access.hpp"
#include "ui/debug_loadout.hpp"

using namespace vl::ui;

namespace vl {
void App::newGame() {
    refresh(); newCharacter=0; newDifficulty=settings.difficulty; newSlot=1;
    for (int i=0;i<3;++i) if (!slots[i]) { newSlot=i+1; break; }
    show(View::create);
}

void App::beginCampaign() {
    Save next;
    next.character=newCharacter; next.difficulty=newDifficulty;
    if (!persist(next,newSlot)) { show(View::create); return; }
    save=next; slot=newSlot; sector=0; debugLoadoutReady=false;
    if (settings.cinematic) {
        film=0; audio.track("coop-intro",false); show(View::intro);
    } else launch(0);
}

void App::loadCampaign(int target) {
    if (!slots[target-1]) return;
    save=*slots[target-1]; slot=target; sector=std::min(save.cleared,sectorCount-1);
    debugLoadoutReady=false;
    showMap();
}

void App::launch(int index) {
    if (!canDeployLevel(index,save.cleared,settings.debugUnlockLevels)) return;
    sector=index;
    game.start(sector,effectiveLoadout());
    r.preload(story::scenery(story::chapters[index].set));
    r.preload("cinematic/deck");
    r.preload("cinematic/aster");
    r.preload("cinematic/recorder");
    audio.preload(story::track(index));
    previousPlayer=game.player; previousCamera=game.camera; previousCameraY=game.cameraY;
    accumulator=0; finished=false;
    input.reset();
    audio.track(sectorTrack(index)); audio.event("launch",.65f);
    show(View::play);
}

void App::retry() {
    game.retry();
    game.lives=game.maxLives; // A manual retry begins a fresh attempt.
    previousPlayer=game.player; previousCamera=game.camera; previousCameraY=game.cameraY;
    accumulator=0; finished=false; input.reset();
    audio.track(sectorTrack(game.index)); show(View::play);
}

void App::initializeDebugLoadout() {
    debugLoadout=makeDebugLoadout(save);
    debugLoadoutReady=true;
}

const Save& App::effectiveLoadout() {
    if (!settings.debugUnlockLevels) return save;
    if (!debugLoadoutReady) initializeDebugLoadout();
    return debugLoadout;
}

void App::openDebug(bool weaponsScreen) {
    if (!settings.debugUnlockLevels) return;
    // With no campaign loaded, offer a non-persistent practice loadout.
    if (!slot && !debugLoadoutReady) { save=Save{}; sector=0; }
    if (weaponsScreen) showArmory();
    else showMap();
}

void App::showMap() { show(View::campaign); }

void App::showArmory() { weapon=effectiveLoadout().equipped; show(View::armory); }

void App::finishMission() {
    if (finished) return;
    finished=true;
    accumulator=0;
    if (game.mode==Mode::failed) {
        if (game.recoverLife()) {
            // The game core respawns after the failed tick, never mid-collision.
            previousPlayer=game.player;
            previousCamera=game.camera; previousCameraY=game.cameraY;
            input.reset(); audio.event("checkpoint");
            inform("LIFE LOST  /  "+std::to_string(game.lives)+" REMAINING");
            finished=false;
            show(View::play);
            return;
        }
        game.lives=0;
        audio.track(""); audio.event("failure"); show(View::failed); return;
    }
    if (slot && !settings.debugUnlockLevels) {
        auto next=save;
        next.cleared=progressAfterCompletion(next.cleared,game.index);
        next.score=total(next.score,game.score); next.credits=total(next.credits,game.score);
        auto& record=next.records[game.index];
        if (!record.time || game.score>record.score || (game.score==record.score && game.time<record.time))
            record={game.time,game.score};
        if (persist(next,slot)) save=next;
    }
    sector=std::min(game.index+1,sectorCount-1);
    audio.event("complete"); beginEpilogue();
}

void App::equip() {
    if (settings.debugUnlockLevels) {
        if (!debugLoadoutReady) initializeDebugLoadout();
        if (weapon>=0 && weapon<int(weaponCount) && weapon!=debugLoadout.equipped) {
            debugLoadout.equipped=weapon; audio.event("switch");
        }
        return;
    }
    if (!save.owned[weapon] || weapon==save.equipped) return;
    auto next=save; next.equipped=weapon;
    if (persist(next,slot)) { save=next; audio.event("switch"); }
}

void App::buyWeapon() {
    if (settings.debugUnlockLevels) return; // All weapons are temporarily owned.
    if (weapon<0 || weapon>=int(weaponCount) || save.owned[weapon] || save.cleared<weapons[weapon].unlock) return;
    const int cost=weaponPrices[weapon];
    if (save.credits<cost) return;
    auto next=save; next.owned[weapon]=true; next.credits-=cost;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(weapons[weapon].name)+" purchased. Equip it to take it into battle.");
}

void App::buySkill(int skill) {
    if (skill<0 || skill>=2) return;
    if (settings.debugUnlockLevels) {
        if (!debugLoadoutReady) initializeDebugLoadout();
        auto& tier=debugLoadout.skills[debugLoadout.character][skill];
        tier=nextDebugRank(tier,characterSkills[debugLoadout.character][skill].maxRank);
        audio.event("upgrade");
        return;
    }
    const int character=save.character;
    const auto& ability=characterSkills[character][skill];
    const int rank=save.skills[character][skill];
    const int price=ability.base+rank*48;
    if (rank>=ability.maxRank || save.credits<price) return;
    auto next=save; ++next.skills[character][skill]; next.credits-=price;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(ability.name)+" upgraded to rank "+std::to_string(rank+1)+".");
}

void App::buy(int rank) {
    if (weapon<0 || weapon>=int(weaponCount) || rank<0 || rank>=int(upgradeCount)) return;
    if (settings.debugUnlockLevels) {
        if (!debugLoadoutReady) initializeDebugLoadout();
        auto& tier=debugLoadout.tiers[weapon][rank];
        tier=nextDebugRank(tier,3);
        audio.event("upgrade");
        return;
    }
    const int tier=save.tiers[weapon][rank];
    const int price=upgradePrice(rank);
    if (!save.owned[weapon] || tier>=3 || save.credits<price) return;
    auto next=save; ++next.tiers[weapon][rank]; next.credits-=price;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(weapons[weapon].shortName)+" "+upgrades[rank].name+" upgraded to rank "+std::to_string(tier+1)+".");
}

void App::buyExtraLife() {
    if (settings.debugUnlockLevels) {
        if (!debugLoadoutReady) initializeDebugLoadout();
        debugLoadout.extraLives=nextDebugRank(debugLoadout.extraLives,maxExtraLives);
        audio.event("upgrade");
        return;
    }
    if (save.extraLives>=maxExtraLives) return;
    const int cost=extraLifePrice(save.extraLives);
    if (save.credits<cost) return;
    auto next=save; ++next.extraLives; next.credits-=cost;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform("Extra life purchased. Missions now start with "+
           std::to_string(startingLives(save.difficulty,save.extraLives))+" lives.");
}

void App::buyPerk(int index) {
    if (index<0 || index>=int(perkCount)) return;
    if (settings.debugUnlockLevels) {
        if (!debugLoadoutReady) initializeDebugLoadout();
        auto& tier=debugLoadout.perks[index];
        tier=nextDebugRank(tier,perks[index].maxRank);
        audio.event("upgrade");
        return;
    }
    const int rank=save.perks[index];
    const int price=perks[index].base+rank*30;
    if (rank>=perks[index].maxRank || save.credits<price) return;
    auto next=save; ++next.perks[index]; next.credits-=price;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(perks[index].name)+" upgraded to rank "+std::to_string(rank+1)+".");
}

int App::upgradePrice(int rank) const {
    return int(std::lround((upgrades[rank].base+save.tiers[weapon][rank]*22)*weapons[weapon].cost));
}

void App::cycleWeapon(int direction) {
    const int next=nextVisibleWeapon(game.guns,game.gun,direction);
    if (next>=0) game.select(next);
}

// Translates polled input into game state. Every action is evaluated each frame so any chord works.
void App::driveGame(float dt) {
    auto apply=[&](Key key,Act act) {
        if (input.pressed(act)) game.input(key,true);
        game.hold(key,input.down(act));
    };
    apply(Key::left,Act::left); apply(Key::right,Act::right); apply(Key::jump,Act::jump);
    apply(Key::fire,Act::fire); apply(Key::automatic,Act::assist); apply(Key::sprint,Act::sprint);
    apply(Key::crouch,Act::crouch); apply(Key::slow,Act::slow); apply(Key::interact,Act::interact);
    if (input.pressed(Act::reload)) game.input(Key::reload,true);
    game.hold(Key::reload,false);
    for (int i=0;i<static_cast<int>(weaponCount);++i) {
        if (!input.pressed(static_cast<Act>(static_cast<int>(Act::weapon1)+i))) continue;
        const int id=visibleWeaponAt(game.guns,i);
        if (id>=0) game.select(id);
    }
    if (input.pressed(Act::next)) cycleWeapon(1);
    if (input.pressed(Act::previous)) cycleWeapon(-1);
    if (mouseAiming) game.pointAt(aimPoint(),mouseMoved);
    mouseMoved=false;
    game.stick(input.aimStick(),input.aimStickActive());
    (void)dt;
}
} // namespace vl
