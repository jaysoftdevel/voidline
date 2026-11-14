#pragma once
#include "render/renderer.hpp"

namespace vl {
// Everything the per-type enemy drawing functions share for one enemy: the colors after burn damage, the screen position
// including hit shove and death sink, and how far along the death animation is.
struct Renderer::EnemyView {
    explicit EnemyView(const Enemy& enemy):e(enemy) {}
    const Enemy& e;
    Color accent,alert=hex(0xff6870),rim,hull,black=hex(0x06080d),plate;
    int sector=0,facing=1;
    float time=0,turn=1,pace=0,breath=0;
    // 0 for a living enemy, 1 once it has charred. Drives the dark, burnt look of corpses.
    float burnt=0;
    // Death time in seconds, or 0 for a living enemy.
    float dead=0;
    Vec center,target,ground;
    float x=0,y=0;
};
}
