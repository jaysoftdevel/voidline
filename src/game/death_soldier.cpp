#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// A defeated soldier: one stagger step back, the knees give way, the body topples about its feet and hits the
// ground with a thud and a puff of dust. The rig reads deathAge to buckle, bend the spine and let the arms go limp.
void Game::updateSoldierCorpse(Corpse& c,float dt) {
    // Slides back while the legs still carry momentum, never past the edge of the platform it stood on.
    if (c.age<.7f) {
        const float slide=c.v.x*(1-c.age/.7f);
        float x=c.body.box.x+slide*dt;
        for (const auto& surface:platforms) {
            const auto& s=surface.box;
            if (std::abs(s.y-c.body.box.bottom())>4 || c.body.box.right()<s.x || c.body.box.x>s.right()) continue;
            x=clamp(x,s.x+4,s.right()-4-c.body.box.w);
        }
        c.body.box.x=x;
    }
    const float topple=smooth((c.age-.30f)/.55f);
    c.angle=c.rest*topple;
    if (!c.settled && c.age>=.88f) {
        c.settled=true;
        const Vec ground{c.body.box.center().x+c.body.fallDir*22.f,c.body.box.bottom()};
        burst(ground,hex(0xb9c3c9),6,70,ParticleKind::puff);
        burst(ground,hex(0x93a5ac),4,95,ParticleKind::chip);
        ring(ground,hex(0xdde5ea),24,.3f);
        shake=std::max(shake,2.6f);
        audio.event("land",.55f,(ground.x-player.box.x)/width,.62f+random()*.1f);
        audio.event("step-deck",.4f,(ground.x-player.box.x)/width,.55f);
    }
    // Damaged armor smolders for a moment after the fall.
    if (c.settled && random()<dt*7) burst(c.body.box.center()+Vec{c.body.fallDir*random()*18.f,-4},hex(0x59616a),1,24,ParticleKind::puff);
}
} // namespace vl
