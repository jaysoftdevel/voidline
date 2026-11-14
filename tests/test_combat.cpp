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
    game.pointer({game.player.box.x - game.camera + 380.f, floorY - 30.f});
    game.input(Key::fire, true);
    for (int i = 0; i < 600 && game.kills < 3; ++i) game.tick();
    // One shell takes out the whole cluster.
    CHECK(game.kills >= 2);
    CHECK(game.guns[5].ammo >= game.guns[5].capacity - 2);
}
}

void runCombatTests() {
    comboScoring();
    reactions();
    explosives();
}
