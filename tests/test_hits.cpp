// Hit reactions, wounds and the death animation of every enemy type.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
Game& arena(Game& game, EnemyKind kind, float distance = 200.f, int hp = 100000) {
    game.start(0, midSave());
    game.entry = 0;
    run(game, 30);
    const float floorY = game.player.box.bottom();
    constexpr Vec sizes[]{{48, 30}, {35, 54}, {48, 46}, {54, 37}, {108, 97}};
    const Vec size = sizes[static_cast<int>(kind)];
    for (size_t i = 0; i < game.enemies.size(); ++i) {
        auto& e = game.enemies[i];
        if (i != 0) { e.box.x = 9000; e.origin.x = 9000; continue; }
        e.kind = kind;
        const float top = kind == EnemyKind::drone ? floorY - 90.f : floorY - size.y;
        e.box = {game.player.box.x + distance, top, size.x, size.y};
        e.origin = {e.box.x, top}; e.base = floorY; e.patrol = 0; e.warning = 0; e.timer = 999; e.damage = 0;
        e.hp = e.maxHp = hp;
    }
    return game;
}

void aimAt(Game& game, const Enemy& e) {
    game.pointer({e.box.center().x - game.camera, e.box.center().y});
}

void woundsAndFeedback() {
    Silent audio;
    Game game(audio);
    arena(game, EnemyKind::ranger);
    auto& e = game.enemies[0];
    aimAt(game, e);
    game.input(Key::fire, true);
    run(game, 90);
    CHECK(e.woundCount >= 2);
    // Wounds are stored relative to the body, inside its bounds.
    for (int i = 0; i < std::min<int>(e.woundCount, 6); ++i) {
        CHECK(std::abs(e.wounds[static_cast<size_t>(i)].local.x) <= e.box.w * .5f + .1f);
        CHECK(std::abs(e.wounds[static_cast<size_t>(i)].local.y) <= e.box.h * .5f + .1f);
    }
    CHECK(e.bar > 0 && e.trail >= static_cast<float>(e.hp) / e.maxHp);
    CHECK(game.hitMarker > 0 || game.killMarker > 0);
    CHECK(!game.particles.empty());
    // The health bar fades out some time after the last hit and the trailing segment catches up.
    game.input(Key::fire, false);
    run(game, 120 * 4);
    CHECK(e.bar == 0);
    CHECK(std::abs(e.trail - static_cast<float>(e.hp) / e.maxHp) < .01f);
}

void bodyPartReactions() {
    Silent audio;
    Game game(audio);
    arena(game, EnemyKind::ranger, 200.f, 100000);
    auto& e = game.enemies[0];
    const Vec dir{1, 0};
    // A head shot snaps the head back, a leg shot buckles the knees.
    game.damageEnemy(e, 10, e.box.center() + Vec{0, -e.box.h * .45f}, dir, false);
    CHECK(e.headSnap > .9f && e.buckle == 0 && e.zone < -.3f);
    game.damageEnemy(e, 10, e.box.center() + Vec{0, e.box.h * .45f}, dir, false);
    CHECK(e.buckle > .9f && e.zone > .3f);
    // Both fade away.
    run(game, 120);
    CHECK(e.headSnap == 0 && e.buckle == 0);
}

void damageNumbers() {
    Silent audio;
    Game game(audio);
    arena(game, EnemyKind::drone);
    auto& e = game.enemies[0];
    const size_t before = game.messages.size();
    // Rapid hits are merged into fewer floating numbers.
    for (int i = 0; i < 6; ++i) { game.damageEnemy(e, 10, e.box.center(), {1, 0}, false); game.tick(); }
    CHECK(game.messages.size() - before < 6);
    run(game, 60);
    int shown = 0;
    for (const auto& m : game.messages) if (!m.text.empty() && std::isdigit(static_cast<unsigned char>(m.text[0]))) ++shown;
    CHECK(shown >= 1);
}

// Kills each type with one big hit and follows the corpse through its animation.
void deaths() {
    struct Case { EnemyKind kind; float minLife; };
    const Case cases[] = {{EnemyKind::ranger, 2.5f}, {EnemyKind::drone, 1.5f}, {EnemyKind::turret, 1.5f},
                          {EnemyKind::stalker, 1.5f}, {EnemyKind::boss, 3.f}};
    for (const auto& c : cases) {
        Silent audio;
        Game game(audio);
        arena(game, c.kind, 200.f, 100);
        auto& e = game.enemies[0];
        game.hitStop = 0;
        game.damageEnemy(e, 500, e.box.center(), {1, 0}, false);
        CHECK(!e.alive && game.kills == 1 && game.killMarker > 0);
        CHECK(game.corpses.size() == 1);
        if (game.corpses.empty()) continue;
        const Corpse& corpse = game.corpses.front();
        CHECK(corpse.life >= c.minLife);
        // The corpse keeps its wound and its own animation clock, and is removed when its life is over.
        const int ticks = static_cast<int>((corpse.life + .5f) * 120);
        bool moved = false;
        const float startAngle = corpse.angle;
        for (int i = 0; i < ticks; ++i) {
            game.tick();
            if (!game.corpses.empty() && std::abs(game.corpses.front().angle - startAngle) > .05f) moved = true;
        }
        CHECK(game.corpses.empty());
        if (c.kind != EnemyKind::turret && c.kind != EnemyKind::boss) CHECK(moved);
    }
}

void soldierTopples() {
    Silent audio;
    Game game(audio);
    arena(game, EnemyKind::ranger, 200.f, 100);
    auto& e = game.enemies[0];
    game.damageEnemy(e, 500, e.box.center(), {1, 0}, false);
    CHECK(game.corpses.size() == 1 && game.corpses.front().pivotFeet);
    // It stays upright at first, then topples toward the side away from the shooter and settles.
    run(game, 12);
    CHECK(std::abs(game.corpses.front().angle) < .2f);
    run(game, 150);
    const auto& corpse = game.corpses.front();
    CHECK(corpse.settled);
    CHECK(std::abs(corpse.angle) > 1.3f);
    CHECK(corpse.angle * corpse.body.fallDir > 0);
    // The rifle leaves the hands as a separate object.
    bool rifle = false;
    for (const auto& p : game.particles) rifle = rifle || p.kind == ParticleKind::weapon;
    CHECK(rifle);
}

void machinesExplode() {
    for (EnemyKind kind : {EnemyKind::drone, EnemyKind::turret}) {
        Silent audio;
        Game game(audio);
        arena(game, kind, 200.f, 100);
        game.damageEnemy(game.enemies[0], 500, game.enemies[0].box.center(), {1, 0}, false);
        bool scorched = false;
        for (int i = 0; i < 300 && !scorched; ++i) { game.tick(); scorched = !game.corpses.empty() && game.corpses.front().scorched; }
        CHECK(scorched);
    }
}

void guardianFinale() {
    Silent audio;
    Game game(audio);
    arena(game, EnemyKind::boss, 300.f, 100);
    game.damageEnemy(game.enemies[0], 500, game.enemies[0].box.center(), {1, 0}, false);
    CHECK(game.bossDead);
    CHECK(game.corpses.size() == 1);
    int blasts = 0;
    float flash = 0;
    for (int i = 0; i < 120 * 5 && !game.corpses.empty(); ++i) {
        game.tick();
        if (!game.corpses.empty()) blasts = std::max(blasts, game.corpses.front().blasts);
        flash = std::max(flash, game.flash);
    }
    // A series of explosions walks across the hull, then a final white flash.
    CHECK(blasts >= 6);
    CHECK(flash >= .5f);
    CHECK(game.corpses.empty());
}
}

void runHitTests() {
    woundsAndFeedback();
    bodyPartReactions();
    damageNumbers();
    deaths();
    soldierTopples();
    machinesExplode();
    guardianFinale();
}
