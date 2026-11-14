#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// A sector guardian does not just vanish. For a few seconds it shudders and sinks while explosions walk across its
// hull, faster and larger as time runs out, and it ends in one big blast with a white flash.
void Game::updateGuardianCorpse(Corpse& c,float dt) {
    const float t=clamp(c.age/c.life);
    const Rect& box=c.body.box;
    c.nextBlast-=dt;
    if (c.nextBlast<=0 && c.age<c.life-.3f) {
        const Vec at{box.x+random()*box.w,box.y+random()*box.h};
        wreckBlast(at,.35f+.5f*t);
        shake=std::max(shake,6.f+8.f*t);
        c.nextBlast=mix(.34f,.11f,t)*(.7f+random()*.6f);
        ++c.blasts;
    }
    if (random()<dt*40) burst({box.x+random()*box.w,box.y+random()*box.h},hex(0xffb36a),1,150);
    if (random()<dt*18) burst({box.x+random()*box.w,box.y+box.h*.3f},hex(0x4b4f56),1,36,ParticleKind::puff);
    if (!c.scorched && c.age>=c.life-.28f) {
        // The final blast fires once.
        c.scorched=true;
        const Vec center=box.center();
        wreckBlast(center,1.2f);
        burst(center,hex(0xffffff),30,520);
        burst(center,hex(0xffb36a),34,420);
        ring(center,hex(0xffffff),200,.7f);
        ring(center,hex(stage->accent),130,.6f);
        flash=std::max(flash,1.f); hitStop=std::max(hitStop,.12f); punch=std::max(punch,1.f);
        shake=std::max(shake,18.f);
    }
}
} // namespace vl
