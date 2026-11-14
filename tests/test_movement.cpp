// Movement, jumping, flipping and determinism.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void chordsDriveTheGame() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    const float startX = game.player.box.x;
    const int ammo = game.guns[game.gun].ammo;
    game.input(Key::jump, true);
    for (Key k : {Key::right, Key::sprint, Key::automatic, Key::slow}) game.input(k, true);
    float scale = 1;
    for (int i = 0; i < 120; ++i) { scale = game.advance(1.f / 120); game.tick(); }
    CHECK(game.player.box.x > startX + 40);
    CHECK(game.guns[game.gun].ammo < ammo);
    CHECK(game.slowMotion());
    CHECK(scale < .9f);
    CHECK(game.slowEnergy < 1.f);
    // Release the chord and the world returns to full speed.
    for (Key k : {Key::right, Key::sprint, Key::automatic, Key::slow, Key::jump}) game.input(k, false);
    for (int i = 0; i < 240; ++i) scale = game.advance(1.f / 120);
    CHECK(scale > .99f);
}
void airFlip() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    run(game, 30);
    CHECK(game.player.support >= 0);
    game.input(Key::jump, true);
    run(game, 60);
    CHECK(game.player.support < 0);
    CHECK(!game.player.airJumpUsed);
    game.input(Key::jump, false);
    const float before = game.player.velocity.y;
    game.input(Key::jump, true);
    run(game, 2);
    CHECK(game.player.airJumpUsed && game.player.flip > 0);
    CHECK(game.player.velocity.y < before);
    // Only one flip per airtime.
    game.input(Key::jump, false);
    run(game, 40);
    game.input(Key::jump, true);
    const float second = game.player.velocity.y;
    run(game, 2);
    CHECK(game.player.velocity.y > second - 1.f);
}
void deterministic() {
    using namespace vl;
    auto simulate = [] {
        Silent audio;
        Game game(audio);
        game.start(1, startSave());
        game.entry = 0;
        game.input(Key::right, true);
        game.input(Key::automatic, true);
        for (int i = 0; i < 600; ++i) {
            if (i % 90 == 10) game.input(Key::jump, true); else if (i % 90 == 40) game.input(Key::jump, false);
            game.tick();
        }
        return std::make_tuple(game.player.box.x, game.player.box.y, game.score, game.kills, game.particles.size());
    };
    CHECK(simulate() == simulate());
}

// Crouch, move and jump held together: the jump must not be swallowed by the crouch, whatever the press order.
void chordWithCrouch() {
    for (int order = 0; order < 2; ++order) {
        Silent audio;
        Game game(audio);
        game.start(0, startSave());
        game.entry = 0;
        run(game, 30);
        const float floorY = game.player.box.bottom();
        if (order == 0) { game.input(Key::crouch, true); game.input(Key::right, true); game.input(Key::sprint, true); }
        else { game.input(Key::right, true); game.input(Key::sprint, true); game.input(Key::crouch, true); }
        run(game, 40);
        CHECK(game.player.crouched);
        game.input(Key::jump, true);
        run(game, 24);
        CHECK(game.player.support < 0 && game.player.box.bottom() < floorY - 4);
        CHECK(!game.player.crouched);
        // The crouch key is still held in the air and after landing. The operative stays up until it is released.
        game.input(Key::right, false); game.input(Key::sprint, false);
        run(game, 240);
        CHECK(game.player.support >= 0 && !game.player.crouched);
        game.input(Key::crouch, false);
        game.input(Key::jump, false);
        run(game, 10);
        CHECK(!game.player.crouched);
        game.input(Key::crouch, true);
        run(game, 20);
        CHECK(game.player.crouched);
    }
}
}

void runMovementTests() {
    chordsDriveTheGame();
    airFlip();
    deterministic();
    chordWithCrouch();
}
