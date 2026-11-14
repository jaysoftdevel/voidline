#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
// Actors are drawn after lighting and fog so their flat, saturated shapes stay crisp against the backdrop.
void Renderer::actors(const Game& g) {
    const Color accent=hex(g.stage->accent);
    const Vec target=Vec{g.player.box.center().x,g.player.box.y+g.player.box.h*.4f}+offset;
    tinting=true;
    actorLight=litBy(target,g);
    drawCorpses(g);
    for (const auto& e:g.enemies) {
        if (!e.alive || !visible(e.box.x,e.box.w)) continue;
        actorLight=litBy(e.box.center()+offset,g);
        drawEnemy(e,accent,g.index,g.time,target);
    }
    actorLight=litBy(target,g);
    drawPlayer(g);
    tinting=false;
    drawRings(g);
}
} // namespace vl
