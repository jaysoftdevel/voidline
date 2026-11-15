// Weapons, hits, combos and explosions.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void comboScoring() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    run(game, 30);
    const auto& body = game.player.box;
    for (int i = 0; i < 2; ++i) {
        auto& e = game.enemies[i];
        e.kind = EnemyKind::turret;
        e.hp = e.maxHp = 1;
        e.box = {body.x + 120.f + i * 70.f, body.bottom() - 46.f, 48, 46};
        e.origin = {e.box.x, e.box.y};
        e.warning = 0; e.timer = 99;
    }
    game.guns[game.gun].damage = 500;
    game.input(Key::automatic, true);
    for (int i = 0; i < 360 && game.kills < 2; ++i) game.tick();
    CHECK(game.kills >= 2);
    CHECK(game.combo >= 2);
    CHECK(game.bestCombo >= 2);
    CHECK(game.hitStop > 0 || game.punch > 0);
    CHECK(!game.rings.empty());
    CHECK(game.score > 0);
    // Defeated enemies leave a falling body that settles, then fades away.
    CHECK(game.corpses.size() >= 2);
    for (int i = 0; i < 1200 && !game.corpses.empty(); ++i) game.tick();
    CHECK(game.corpses.empty());
}
void momentumRewards() {
    Silent audio;
    Game game(audio);
    game.start(0,fullSave());
    game.entry=0;
    game.slowEnergy=0;
    auto& magazine=game.guns[game.gun];
    magazine.ammo=std::max(0,magazine.capacity-8);
    const int before=magazine.ammo;
    for (int n=0;n<5;++n) {
        auto& enemy=game.enemies[n];
        enemy.hp=enemy.maxHp=1;
        game.damageEnemy(enemy,20,enemy.box.center(),{1,0},false);
    }
    CHECK(game.combo==5 && game.bestCombo==5);
    CHECK(game.slowEnergy>.45f); // Kill returns plus the three-kill momentum surge.
    CHECK(magazine.ammo>before && magazine.ammo<=magazine.capacity);
    CHECK(game.score>0);
    CHECK(std::any_of(game.messages.begin(),game.messages.end(),[](const auto& m) {
        return m.text.find("MOMENTUM")!=std::string::npos;
    }));
    CHECK(std::any_of(game.messages.begin(),game.messages.end(),[](const auto& m) {
        return m.text.find("AMMO SURGE")!=std::string::npos;
    }));
    game.tick(3.3f);
    CHECK(game.combo==0); // A broken streak must not carry rewards to the next encounter.
}
void reactions() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    run(game, 30);
    auto& e = game.enemies[0];
    e.kind = EnemyKind::ranger;
    e.hp = e.maxHp = 100000;
    e.box = {game.player.box.x + 140.f, game.player.box.bottom() - 54.f, 35, 54};
    e.origin = {e.box.x, e.box.y}; e.base = game.player.box.bottom(); e.patrol = 0;
    game.input(Key::automatic, true);
    float peakFlinch = 0, raise = 0;
    for (int i = 0; i < 240; ++i) { game.tick(); peakFlinch = std::max(peakFlinch, e.flinch); raise = std::max(raise, e.raise); }
    CHECK(peakFlinch > .5f);
    CHECK(raise > .9f);
    // The operative turns smoothly instead of snapping.
    game.input(Key::automatic, false);
    game.pointer({100, 300});
    run(game, 4);
    CHECK(std::abs(game.player.turn) < 1.f);
    run(game, 120);
    CHECK(game.player.turn == -1.f);
}
void explosives() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, fullSave());
    game.entry = 0;
    run(game, 30);
    const float floorY = game.player.box.bottom();
    for (int i = 0; i < 3; ++i) {
        auto& e = game.enemies[i];
        e.kind = EnemyKind::turret;
        e.hp = e.maxHp = 40;
        e.box = {game.player.box.x + 340.f + i * 30.f, floorY - 46.f, 48, 46};
        e.origin = {e.box.x, e.box.y}; e.warning = 0; e.timer = 99;
    }
    game.select(5);
    CHECK(game.gun == 5);
    game.pointAt({game.player.box.x + 380.f, floorY - 30.f},true); // World-space aiming independent of camera scroll.
    game.input(Key::fire, true);
    for (int i = 0; i < 600 && game.kills < 3; ++i) game.tick();
    // One shell takes out the whole cluster.
    CHECK(game.kills >= 2);
    CHECK(game.guns[5].ammo >= game.guns[5].capacity - 2);
}

void coverAndCrates() {
    Silent audio;
    Game game(audio);
    auto start=[&] {
        game.start(0,fullSave());
        for (auto& e:game.enemies) e.alive=false;
        game.crates.clear();
        game.entry=0;
    };
    auto bullet=[&](float x,float y) {
        game.shots.push_back({{x,y},{1200,0},1,20,hex(0xffffff)});
        run(game,10);
    };

    start();
    game.platforms.push_back({{147,462,44,43},{},false,nullptr,true});
    game.input(Key::crouch,true);
    game.tick();
    CHECK(game.player.crouched && game.nearCover());
    const auto hp=game.player.hp;
    game.hostile.push_back({{210,481},{-1200,0},1,20,hex(0xffffff)});
    run(game,12);
    CHECK(game.player.hp==hp && game.hostile.empty());
    game.input(Key::crouch,false);
    game.tick();
    CHECK(game.weaponPivot().y<462);
    game.hostile.push_back({{210,455},{-1200,0},1,20,hex(0xffffff)});
    run(game,12);
    CHECK(game.player.hp<hp);

    for (const auto kind:{CrateKind::charge,CrateKind::fuel,CrateKind::reactor}) {
        start();
        const auto& spec=crateSpec(kind);
        game.crates.push_back({kind,{153,472,36,33},spec.hits});
        auto& e=game.enemies.front();
        e.alive=true; e.kind=EnemyKind::ranger; e.hp=e.maxHp=200;
        e.box={202,451,35,54}; e.origin={202,451}; e.patrol=0; e.damage=0; e.reach=0;
        for (int hits=spec.hits-1;hits>0;--hits) {
            bullet(126,488);
            CHECK(!game.crates.front().spent && game.crates.front().hits==hits);
            CHECK(game.player.hp==game.maxHealth && e.hp==200);
        }
        bullet(126,488);
        CHECK(game.crates.front().spent);
        CHECK(game.player.hp<game.maxHealth && e.hp<200);
    }

    start();
    game.crates={{CrateKind::reactor,{242,472,36,33},7},{CrateKind::charge,{153,472,36,33},1}};
    bullet(126,488);
    CHECK(game.crates[0].spent && game.crates[1].spent);
}

void piercingBeyondThirtyTwoEnemies() {
    Silent audio;
    Game game(audio);
    game.start(sectorCount-1,fullSave());
    CHECK(game.enemies.size()>32);
    for (auto& e:game.enemies) e.alive=false;
    game.crates.clear();
    // This test isolates the 33rd enemy piercing path from stage geometry.
    std::erase_if(game.platforms,[](const Platform& p){return !p.ground;});
    auto& first=game.enemies[0];
    auto& later=game.enemies[32];
    auto place=[](Enemy& e,float x) {
        e.alive=true; e.kind=EnemyKind::turret; e.hp=e.maxHp=100;
        e.box={x,432,31,36}; e.origin={x,432}; e.damage=0; e.reach=0;
    };
    const float floor=game.stage->ground.front().y;
    place(first,250); place(later,290);
    first.box.y=floor-43; later.box.y=floor-43;
    game.shots.push_back({{244,floor-25},{1200,0},1,20,hex(0xffffff),1});
    run(game,10);
    CHECK(first.hp==80 && later.hp==80);
}

}

void runCombatTests() {
    comboScoring();
    momentumRewards();
    reactions();
    explosives();
    coverAndCrates();
    piercingBeyondThirtyTwoEnemies();
}
