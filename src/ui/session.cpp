#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"

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
    save=next; slot=newSlot; sector=0;
    if (settings.cinematic) {
        film=0; audio.track("coop-intro",false); show(View::intro);
    } else launch(0);
}

void App::loadCampaign(int target) {
    if (!slots[target-1]) return;
    save=*slots[target-1]; slot=target; sector=std::min(save.cleared,sectorCount-1);
    showMap();
}

void App::launch(int index) {
    sector=index;
    game.start(sector,save);
    r.preload(story::scenery(story::chapters[index].set));
    r.preload("cinematic/deck");
    r.preload("cinematic/aster");
    r.preload("cinematic/recorder");
    audio.preload(story::track(index));
    previousPlayer=game.player; previousCamera=game.camera;
    accumulator=0; finished=false;
    input.reset();
    audio.track(sectorTrack(index)); audio.event("launch",.65f);
    show(View::play);
}

void App::retry() {
    game.retry();
    previousPlayer=game.player; previousCamera=game.camera;
    accumulator=0; finished=false; input.reset();
    audio.track(sectorTrack(game.index)); show(View::play);
}

void App::showMap() { show(View::campaign); }

void App::showArmory() { weapon=save.equipped; show(View::armory); }

void App::finishMission() {
    if (finished) return;
    finished=true;
    accumulator=0;
    if (game.mode==Mode::failed) {
        audio.track(""); audio.event("failure"); show(View::failed); return;
    }
    if (slot) {
        auto next=save;
        next.cleared=std::max(next.cleared,game.index+1);
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
    if (save.cleared<weapons[weapon].unlock || weapon==save.equipped) return;
    auto next=save; next.equipped=weapon;
    if (persist(next,slot)) { save=next; audio.event("switch"); }
}

void App::buy(int rank) {
    const int tier=save.tiers[weapon][rank];
    const int price=upgradePrice(rank);
    if (save.cleared<weapons[weapon].unlock || tier>=3 || save.credits<price) return;
    auto next=save; ++next.tiers[weapon][rank]; next.credits-=price;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(weapons[weapon].shortName)+" "+upgrades[rank].name+" upgraded to rank "+std::to_string(tier+1)+".");
}

void App::buyPerk(int index) {
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
    for (int hop=1;hop<=static_cast<int>(weaponCount);++hop) {
        const int count=static_cast<int>(weaponCount);
        const int index=((game.gun+direction*hop)%count+count)%count;
        if (game.guns[index].available) { game.select(index); return; }
    }
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
    for (int i=0;i<static_cast<int>(weaponCount);++i) if (input.pressed(static_cast<Act>(static_cast<int>(Act::weapon1)+i))) game.select(i);
    if (input.pressed(Act::next)) cycleWeapon(1);
    if (input.pressed(Act::previous)) cycleWeapon(-1);
    if (mouseAiming) game.pointAt(aimPoint(),mouseMoved);
    mouseMoved=false;
    game.stick(input.aimStick(),input.aimStickActive());
    (void)dt;
}
} // namespace vl
