#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"
#include "render/figures/humanoid.hpp"

using namespace vl::figures;

namespace vl {
void Renderer::drawCorpses(const Game& g) {
    const Color accent=hex(g.stage->accent);
    for (const auto& c:g.corpses) {
        if (!visible(c.body.box.x,c.body.box.w)) continue;
        const float remaining=c.life-c.age;
        if (remaining<.4f && std::fmod(c.age*24,2.f)<1.f) continue;
        const Vec center=c.body.box.center()+offset;
        // Soldiers topple about their feet, everything else tumbles about its center.
        const Vec pivot=c.pivotFeet?Vec{center.x,c.body.box.bottom()+offset.y}:center;
        beginSpin(pivot,c.angle);
        drawEnemy(c.body,accent,g.stage->biome,g.time,center);
        endSpin();
    }
}
} // namespace vl
