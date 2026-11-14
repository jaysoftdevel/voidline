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
        drawEnemy(e,accent,g.stage->biome,g.time,target);
    }
    if (g.coop) {
        const auto& partner=g.teammate();
        if (visible(partner.player.box.x,partner.player.box.w)) {
            const Vec shift=playerShift;
            const auto heels=drawHeels;
            const auto angles=drawFootAngles;
            const auto gait=pose;
            const float ready=carry;
            operativePose(partner.player,priorPartner?*priorPartner:partner.player,partnerBlend);
            actorLight=litBy(partner.player.box.center()+offset,g);
            drawPlayer(g,partner);
            const Vec label{partner.player.box.center().x+offset.x+playerShift.x,
                            partner.player.box.y+offset.y+playerShift.y-14};
            outlinedText(label-Vec{measure("PARTNER",10,true)*.5f,0},"PARTNER",10,hex(partner.character->color));
            playerShift=shift; drawHeels=heels; drawFootAngles=angles; pose=gait; carry=ready;
        }
    }
    actorLight=litBy(target,g);
    drawPlayer(g,g.operative);
    tinting=false;
    drawRings(g);
}
} // namespace vl
