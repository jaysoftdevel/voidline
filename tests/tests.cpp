// Headless checks for input chords and simulation rules. The game core does not need a window or audio device.
#include "game.hpp"
#include "input.hpp"
#include <cstdlib>
#include <iostream>
#include <tuple>

namespace {
int failures = 0;
void check(bool ok, const char* what, int line) {
    if (ok) return;
    ++failures;
    std::cerr << "FAILED line " << line << ": " << what << '\n';
}
#define CHECK(expr) check((expr), #expr, __LINE__)

struct Silent final : vl::AudioSink {
    int events = 0;
    void event(std::string_view, float, float, float) override { ++events; }
};

SDL_Event keyEvent(unsigned type, int scancode) {
    SDL_Event e{};
    e.type = type;
    e.key.keysym.scancode = scancode;
    return e;
}

void run(vl::Game& game, int ticks) {
    for (int i = 0; i < ticks; ++i) game.tick();
}

void inputChords() {
    using namespace vl;
    Input input;
    // Eight simultaneous keys from different actions, plus a mouse button.
    for (int k : {sdl::key::d, sdl::key::space, sdl::key::shiftLeft, sdl::key::f, sdl::key::q, sdl::key::c, sdl::key::r, sdl::key::two})
        input.inject(k, true);
    input.injectButtons(sdl::buttonLeft);
    input.poll();
    CHECK(input.down(Act::right));
    CHECK(input.down(Act::jump));
    CHECK(input.down(Act::sprint));
    CHECK(input.down(Act::assist));
    CHECK(input.down(Act::slow));
    CHECK(input.down(Act::crouch));
    CHECK(input.down(Act::reload));
    CHECK(input.down(Act::weapon2));
    CHECK(input.down(Act::fire));
    CHECK(!input.down(Act::left));
    CHECK(input.heldKeys() >= 8);
    CHECK(input.pressed(Act::jump));
    input.poll();
    CHECK(!input.pressed(Act::jump) && input.down(Act::jump));

    // Releasing one key must not disturb the others.
    input.inject(sdl::key::space, false);
    input.poll();
    CHECK(!input.down(Act::jump) && input.released(Act::jump));
    CHECK(input.down(Act::right) && input.down(Act::sprint) && input.down(Act::slow));

    // Alternate bindings.
    input.reset();
    input.inject(sdl::key::left, true);
    input.inject(sdl::key::up, true);
    input.inject(sdl::key::controlLeft, true);
    input.poll();
    CHECK(input.down(Act::left) && input.down(Act::jump) && input.down(Act::crouch));
    CHECK(input.horizontal() < 0);

    // A tap shorter than one frame is still reported as a press.
    input.reset();
    input.event(keyEvent(sdl::keyDown, sdl::key::k));
    input.event(keyEvent(sdl::keyUp, sdl::key::k));
    input.poll();
    CHECK(input.pressed(Act::jump) && !input.down(Act::jump));
    input.poll();
    CHECK(!input.pressed(Act::jump));
}

vl::Save startSave() {
    vl::Save save;
    save.cleared = 3;
    return save;
}

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

void stageData() {
    using namespace vl;
    for (const auto& stage : stages) {
        CHECK(stage.worldWidth > stage.exit.x);
        CHECK(!stage.ground.empty());
        CHECK(!stage.enemies.empty() && stage.enemies.back().kind == EnemyKind::boss);
    }
}
}

int main() {
    inputChords();
    chordsDriveTheGame();
    airFlip();
    comboScoring();
    reactions();
    deterministic();
    stageData();
    if (failures) { std::cerr << failures << " check(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "All checks passed\n";
    return EXIT_SUCCESS;
}
