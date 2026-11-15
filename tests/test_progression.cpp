// Upgrades, perks and campaign data.
#include "game/detail.hpp"
#include "support.hpp"
#include "services/storage.hpp"
#include "content/level_io.hpp"
#include "platform/paths.hpp"
#include <filesystem>
#include <chrono>

using namespace test;
using namespace vl;

namespace {
void fpsSettings() {
    const auto path=std::filesystem::temp_directory_path()/
        ("voidline-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
    Storage storage(path);
    auto options=storage.settings();
    CHECK(!options.fpsCounter && options.fpsCorner==1);
    options.fpsCounter=true; options.fpsCorner=3;
    storage.put(options);
    auto read=storage.settings();
    CHECK(read.fpsCounter && read.fpsCorner==3);
    read.fpsCounter=false; read.fpsCorner=0; storage.put(read);
    read=storage.settings();
    CHECK(!read.fpsCounter && read.fpsCorner==0);
    auto& q=read.graphics;
    q=graphicsPreset(3);
    CHECK(q.preset==3 && q.resolution==4 && q.shadows==3 && q.reflections==3 && !q.dynamicResolution);
    q.preset=4; q.materials=1; q.fog=0; q.antialias=0; q.water=1;
    storage.put(read);
    const auto saved=storage.settings().graphics;
    CHECK(saved.preset==4 && saved.materials==1 && saved.fog==0 && saved.antialias==0 && saved.water==1);
    CHECK(saved.resolution==4 && saved.shadows==3 && !saved.dynamicResolution);
    const auto fast=graphicsPreset(0);
    CHECK(fast.preset==0 && fast.reflections==0 && fast.bloom==0 && fast.particles==0);
    const auto middle=graphicsPreset(1);
    CHECK(middle.preset==1 && middle.lighting==1 && middle.resolution==2);
    const auto ultra=graphicsPreset(3);
    CHECK(ultra.preset==3 && ultra.lighting==3 && ultra.grading==3);
    std::filesystem::remove(path);
    std::filesystem::remove(path.string()+".lock");
}
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
void rosterAndCharacters() {
    CHECK(characters.size()==6);
    CHECK(spec(EnemyKind::scavenger).hp < spec(EnemyKind::sniper).hp);
    CHECK(spec(EnemyKind::sniper).hp < spec(EnemyKind::bulwark).hp);
    CHECK(spec(EnemyKind::bulwark).hp < spec(EnemyKind::juggernaut).hp);
    Silent audio;
    Game game(audio);
    for (int c=0;c<static_cast<int>(characters.size());++c) {
        Save save=fullSave(); save.character=c;
        game.start(0,save);
        CHECK(game.character==&characters[c]);
        CHECK(game.maxHealth==characters[c].health);
        CHECK(game.player.hp==game.maxHealth);
        CHECK(game.guns[0].maxReserve==int(std::lround(weapons[0].reserve*characters[c].ammo)));
    }
    constexpr EnemyKind newKinds[]{EnemyKind::scavenger,EnemyKind::sniper,EnemyKind::bulwark,
                                   EnemyKind::interceptor,EnemyKind::juggernaut};
    constexpr int firstSectors[]{1,4,7,10,15};
    for (int kind=0;kind<5;++kind) {
        int first=-1;
        for (int sector=0;sector<sectorCount;++sector)
            for (const auto& e:stages[sector].enemies)
                if (e.kind==newKinds[kind] && first<0) first=sector+1;
        CHECK(first>=firstSectors[kind]);
        CHECK(first<=std::min(sectorCount,firstSectors[kind]+3));
    }
    int earlyHeavy=0,lateHeavy=0;
    for (int sector=0;sector<sectorCount;++sector) {
        int heavy=0;
        for (const auto& e:stages[sector].enemies)
            if (e.kind==EnemyKind::bulwark || e.kind==EnemyKind::juggernaut ||
                e.kind==EnemyKind::interceptor || e.kind==EnemyKind::sniper) ++heavy;
        if (sector<5) earlyHeavy+=heavy;
        if (sector>=15) lateHeavy+=heavy;
    }
    CHECK(lateHeavy>earlyHeavy*3);
    game.start(15,fullSave());
    auto& e=game.enemies[0];
    e.kind=EnemyKind::bulwark; e.facing=-1;
    e.hp=e.maxHp=500;
    game.damageEnemy(e,100,e.box.center(),{1,0},false);
    const int frontal=500-e.hp;
    e.hp=500;
    game.damageEnemy(e,100,e.box.center(),{-1,0},false);
    CHECK(frontal<500-e.hp);
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
        CHECK(stage.worldWidth>=5800.f+730.f*i);
        CHECK(stage.ledges.size()>=size_t(12+2*i));
        CHECK(stage.enemies.size()<=128); // Bounded by multiplayer snapshot serialization.

        CHECK(stage.enemies.size()>=size_t(11+2*i));
        if (i>0) CHECK(stage.worldWidth-length>=390.f);
        if (i>0) CHECK(stage.enemies.size()-enemies>=2);
        length=stage.worldWidth;
        enemies=stage.enemies.size();
        CHECK(!stage.ground.empty());
        CHECK(stage.enemies.back().kind==EnemyKind::boss);
        CHECK(!stage.cover.empty() && !stage.crates.empty());
        CHECK(stage.beacons.size()>=2);
        CHECK(stage.ground.back().right()+.1f>=stage.worldWidth);
        for (size_t g=1;g<stage.ground.size();++g) {
            const float gap=stage.ground[g].x-stage.ground[g-1].right();
            CHECK(gap>=-0.1f && gap<=420.1f);
        }
        for (const auto& placement:stage.enemies) {
            CHECK(placement.x>=0.f && placement.x<stage.worldWidth);
        }
        for (const auto& beacon:stage.beacons) {
            CHECK(std::any_of(stage.ground.begin(),stage.ground.end(),[&](const Rect& g) {
                return std::abs(g.y-beacon.y)<.01f && g.x+16<beacon.x && beacon.x<g.right()-16;
            }));
            const Rect standing{beacon.x-16,beacon.y-56,32,56};
            const bool clearBeacon=std::none_of(stage.cover.begin(),stage.cover.end(),[&](Rect b) { return b.overlaps(standing); });
            if (!clearBeacon) std::cerr << "blocked beacon in sector " << i+1 << " at " << beacon.x << "," << beacon.y << "\n";
            CHECK(clearBeacon);
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

// Verify custom levels still load in the game while built-in progression tests
// remain independent of published levels in the actual game runtime folder.
void authoredLevelOverride() {
    const auto file=levelFile(executableDirectory()/"levels",0,false);
    // This is the isolated test runner; never overwrite a user's real level.
    if (std::filesystem::exists(file)) {
        CHECK(!"Test runner directory contains a pre-existing level override");
        return;
    }
    Stage edited=stages[0];
    edited.worldWidth+=16.f;
    std::string error;
    if (!writeLevel(file,edited,0,false,&error)) {
        std::cerr<<"Could not write isolated test level: "<<error<<'\n';
        CHECK(false);
        return;
    }
    Silent audio;
    Game game(audio);
    game.start(0,fullSave());
    CHECK(game.stage!=&stages[0]);
    CHECK(game.stage->worldWidth==edited.worldWidth);
    std::error_code ec;
    std::filesystem::remove(file,ec);
    CHECK(!ec);
    game.start(0,fullSave());
    CHECK(game.stage==&stages[0]);
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
    fpsSettings();
    upgradesAndPerks();
    rosterAndCharacters();
    stageData();
    checkpointRestore();
    authoredLevelOverride();
}
