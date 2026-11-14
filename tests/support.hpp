#pragma once
// Shared helpers for the headless tests. The game core needs neither a window nor an audio device.
#include "game/game.hpp"
#include "input/input.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <tuple>

namespace test {
inline int failures = 0;
inline void check(bool ok, const char* what, const char* file, int line) {
    if (ok) return;
    ++failures;
    std::cerr << "FAILED " << file << ":" << line << ": " << what << '\n';
}
#define CHECK(expr) ::test::check((expr), #expr, __FILE__, __LINE__)

struct Silent final : vl::AudioSink {
    int events = 0;
    void event(std::string_view, float, float, float) override { ++events; }
};

inline SDL_Event keyEvent(unsigned type, int scancode) {
    SDL_Event e{};
    e.type = type;
    e.key.keysym.scancode = scancode;
    return e;
}

inline void run(vl::Game& game, int ticks) {
    for (int i = 0; i < ticks; ++i) game.tick();
}

inline vl::Save startSave() { return {}; }
inline vl::Save fullSave() {
    vl::Save save;
    save.cleared = 5;
    return save;
}
inline vl::Save midSave() {
    vl::Save save;
    save.cleared = 3;
    return save;
}
}

void runInputTests();
void runMovementTests();
void runCombatTests();
void runHitTests();
void runProgressionTests();
void runWorldTests();
