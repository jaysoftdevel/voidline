// Upgrades, perks and campaign data.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void upgradesAndPerks() {
    using namespace vl;
    Silent audio;
    Game base(audio), tuned(audio);
    base.start(0, fullSave());
    Save save = fullSave();
    for (size_t g = 0; g < weaponCount; ++g) save.tiers[g] = {3, 3, 3, 3, 3, 3, 3, 3};
    save.perks = {4, 4, 4, 4, 4, 4};
    tuned.start(0, save);
    for (size_t g = 0; g < weaponCount; ++g) {
        CHECK(base.guns[g].available && tuned.guns[g].available);
        CHECK(tuned.guns[g].damage > base.guns[g].damage);
        CHECK(tuned.guns[g].range > base.guns[g].range * 1.4f);
        CHECK(tuned.guns[g].interval < base.guns[g].interval);
        CHECK(tuned.guns[g].capacity > base.guns[g].capacity);
        CHECK(tuned.guns[g].spread <= base.guns[g].spread);
        CHECK(tuned.guns[g].crit > 0.f);
    }
    CHECK(tuned.maxHealth > base.maxHealth);
    CHECK(tuned.player.hp == tuned.maxHealth);
    CHECK(tuned.slowDrain < base.slowDrain && tuned.slowRegen > base.slowRegen);
    CHECK(tuned.damageTaken < base.damageTaken);
    // Every original gun reaches farther than before (760, 365, 970 and 680).
    CHECK(weapons[0].range > 760 && weapons[1].range > 365 && weapons[2].range > 970 && weapons[3].range > 680);
}
void stageData() {
    float length=0;
    size_t enemies=0;
    Silent audio;
    Game game(audio);
    for (int i=0;i<sectorCount;++i) {
        const auto& stage=stages[i];
        CHECK(stage.worldWidth>length && stage.worldWidth>stage.exit.x);
        CHECK(stage.enemies.size()>enemies);
        length=stage.worldWidth;
        enemies=stage.enemies.size();
        CHECK(!stage.ground.empty());
        CHECK(stage.enemies.back().kind==EnemyKind::boss);
        CHECK(!stage.cover.empty() && !stage.crates.empty());
        CHECK(!stage.beacons.empty());
        for (const auto& beacon:stage.beacons) {
            CHECK(std::any_of(stage.ground.begin(),stage.ground.end(),[&](const Rect& g) {
                return std::abs(g.y-beacon.y)<.01f && g.x+16<beacon.x && beacon.x<g.right()-16;
            }));
            const Rect standing{beacon.x-16,beacon.y-56,32,56};
            CHECK(std::none_of(stage.cover.begin(),stage.cover.end(),[&](Rect b) { return b.overlaps(standing); }));
        }
        game.start(i,fullSave());
        CHECK(game.stage==&stage);
        game.player.box.x=stage.exit.x-16;
        game.player.box.y=stage.exit.y-game.player.box.h;
        game.player.support=int(stage.ground.size())-1;
        game.tick();
        CHECK(game.mode==Mode::active);
        for (size_t j=0;j+1<game.enemies.size();++j) game.enemies[j].alive=false;
        auto& guardian=game.enemies.back();
        game.damageEnemy(guardian,float(guardian.hp),guardian.box.center(),{1,0},false);
        game.player.box.x=stage.exit.x-68;
        game.player.box.y=stage.exit.y-game.player.box.h;
        game.player.velocity={};
        game.hold(Key::right,true);
        run(game,65);
        CHECK(game.mode==Mode::complete);
    }
}

void checkpointRestore() {
    Silent audio;
    Game game(audio);
    game.start(sectorCount-1,fullSave());
    const auto& beacons=game.stage->beacons;
    auto enter=[&](Vec at) {
        game.player.box.x=at.x-16;
        game.player.box.y=at.y-game.player.box.h;
        game.player.velocity={};
        game.player.support=-1;
        game.tick();
    };
    enter(beacons.front());
    CHECK(game.activeBeacon==0);
    game.score=45;
    game.crates.front().hits=1;
    enter(beacons.back());
    CHECK(game.activeBeacon==int(beacons.size())-1);
    game.score=90;
    game.crates.front().spent=true;
    enter(beacons.front());
    game.retry();
    CHECK(game.mode==Mode::active && game.player.hp==game.maxHealth);
    CHECK(std::abs(game.player.box.center().x-beacons.back().x)<.1f);
    CHECK(game.score==45);
    CHECK(!game.crates.front().spent && game.crates.front().hits==1);
}
}

void runProgressionTests() {
    upgradesAndPerks();
    stageData();
    checkpointRestore();
}
