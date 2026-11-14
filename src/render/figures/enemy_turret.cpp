#include "render/figures/enemy_view.hpp"

namespace vl {
// A servo turret on a splayed tripod. The gimbal tracks smoothly and the barrels kick back when it fires.
// When it dies the gimbal droops, sparks fly, and the barrels are blown off by a secondary explosion.
void Renderer::drawTurret(const EnemyView& v) {
    const Enemy& e=v.e;
    const Color black=v.black,hull=v.hull,rim=v.rim,plate=v.plate,alert=v.alert,accent=v.accent;
    const Vec center=v.center;
    const Vec base=v.ground;
    for (float side:{-1.f,0.f,1.f}) {
        const Vec foot=base+Vec{side*26.f,0};
        const Vec knee=base+Vec{side*15.f,-15.f+std::abs(side)*3.f};
        line(foot,knee,5.f,black); line(knee,center+Vec{side*6.f,10.f},5.f,black);
        line(foot,knee,3.2f,hull); line(knee,center+Vec{side*6.f,10.f},3.2f,hull);
        ellipse(knee,{2.5f,2.5f},plate);
        line(foot-Vec{3.f,0},foot+Vec{3.f,0},2.4f,black);
    }
    polygon(base+Vec{-20.f,0},base+Vec{20.f,0},base+Vec{14.f,-7.f},base+Vec{-14.f,-7.f},hull.energy(.8f));
    line(base+Vec{-20.f,0},base+Vec{20.f,0},1.f,rim.alpha(.6f));
    const Vec mount=center+Vec{0,8.f};
    ellipse(mount,{14.f,6.f},black); ellipse(mount,{12.f,4.6f},plate);
    const float idleAngle=v.facing>0?-.15f:pi+.15f;
    Vec dirv=e.warning>0?ray(e.aim):normal(mix(ray(idleAngle),normal(v.target-center),smooth(e.raise)));
    // The gimbal droops toward the ground and twitches while it dies.
    if (!e.alive) {
        const float droop=smooth(v.dead/.5f);
        const float twitch=std::sin(v.time*33.f)*.08f*(1-droop);
        dirv=ray(idleAngle+(v.facing>0?(.95f+twitch):-(.95f+twitch))*droop);
    }
    const Vec perp{-dirv.y,dirv.x};
    const float kick=e.recoil*6.f;
    const bool blownOff=!e.alive && v.dead>.55f;
    ellipse(center,{15.5f,13.f},black);
    ellipse(center-Vec{0,1.f},{13.f,10.6f},hull);
    ellipse(center-Vec{0,3.f},{10.f,5.f},plate);
    for (int k=-1;k<=1;k+=2) {
        const Vec o=perp*(4.2f*k);
        if (blownOff) {
            // Torn mounting stubs.
            line(center+dirv*4.f+o,center+dirv*8.f+o+perp*k*1.5f,3.2f,black);
            continue;
        }
        line(center+dirv*(4.f-kick)+o,center+dirv*(25.f-kick)+o,3.6f,black);
        line(center+dirv*(4.f-kick)+o,center+dirv*(25.f-kick)+o,2.2f,rim.energy(.75f));
        line(center+dirv*(23.f-kick)+o,center+dirv*(28.f-kick)+o,4.2f,black);
        line(center+dirv*(24.f-kick)+o,center+dirv*(26.5f-kick)+o,2.4f,plate);
    }
    line(center+dirv*8.f-perp*7.f,center+dirv*8.f+perp*7.f,2.4f,black.alpha(.8f));
    // Ammunition belt feeding the barrels.
    for (int k=0;k<5;++k) {
        const Vec bead=center-perp*8.f-dirv*(2.f+k*3.f)+Vec{0,k*1.5f};
        ellipse(bead,{1.3f,1.3f},hex(0xf0c46b).energy(1.1f*(1-v.burnt)));
    }
    const Vec eye=center+perp*(-7.f)+dirv*5.f;
    ellipse(eye,{2.4f,2.4f},black);
    emissive(eye,1.9f,alert,e.warning>0?3.1f:1.4f);
    emissive(center,3.2f,alert,e.warning>0?3.1f:1.4f);
    if (e.recoil>.5f) halo(center+dirv*30.f,18,accent.energy(2.f).alpha(.4f*e.recoil));
    // Exposed wiring and a dented dome when badly hurt.
    if (e.alive && e.hp<e.maxHp*.5f) {
        line(center+Vec{-5.f,-8.f},center+Vec{-9.f,-3.f},.9f,hex(0xffc27a).energy(1.6f));
        line(center+Vec{-9.f,-3.f},center+Vec{-6.f,1.f},.9f,hex(0xffc27a).energy(1.6f));
    }
}
} // namespace vl
