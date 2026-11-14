#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
// Cones of light under lamps. They are drawn before lighting and fog, so they pick up haze and bloom.
void Renderer::shafts(const Game& g) {
    if (graphics.shafts==0) return;
    const Color accent=hex(g.stage->accent),cold=hex(g.stage->cold);
    for (int i=0;140+i*273<g.stage->worldWidth+215;i+=graphics.shafts==1?4:2) {
        const float x=140+i*273+hash(i+g.index*32)*90;
        if (!visible(x,160)) continue;
        const float y=253+hash(i+54)*126;
        const Vec lamp=Vec{x+11,y+8}+offset;
        const float flicker=(.85f+.15f*std::sin(g.time*2.3f+i)+(hash(int(g.time*9)+i)>.97f?-.35f:0.f))*(graphics.shafts==3?1.22f:graphics.shafts==1?.65f:1.f);
        const float reach=210+hash(i+3)*60;
        const Vec lean{.12f*reach,reach};
        beam(lamp,7,lamp+lean,150,accent.energy(1.5f).alpha(.20f*flicker),accent.alpha(0));
        beam(lamp,4,lamp+lean*.8f+Vec{16,0},88,hex(0xfff0d8).alpha(.10f*flicker),accent.alpha(0));
    }
    for (int i=0;167+511*i<g.stage->worldWidth+215;i+=graphics.shafts==1?2:1) {
        const float x=167+511*i;
        if (!visible(x,260)) continue;
        const Vec lamp=Vec{x,143+hash(i+43)*102}+offset;
        const float flicker=.9f+.1f*std::sin(g.time*1.6f+i*2.f);
        beam(lamp,10,lamp+Vec{-.19f*330,330},210,cold.energy(1.6f).alpha(.17f*flicker),cold.alpha(0));
    }
    // Beam from the muzzle flash while firing: a brief cone through the haze.
    const auto& p=g.player;
    if (p.muzzle>0) {
        const float f=p.muzzle/.069f;
        const Vec m=g.muzzle()+offset+playerShift;
        const Color c=hex(weapons[g.gun].color);
        beam(m,5,m+ray(p.aim)*220,60,c.energy(2.f).alpha(.30f*f),c.alpha(0));
    }
}
} // namespace vl
