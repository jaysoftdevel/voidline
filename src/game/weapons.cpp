#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::reload() {
    auto& g=guns[gun];
    if (g.reload>0 || g.ammo>=g.capacity || g.reserve==0) return;
    g.reload=g.duration;
    audio.event("reload",1,0,clamp(.73f/g.duration,.58f,1.65f));
}

void Game::shoot(float dt) {
    for (auto& g:guns) {
        if (g.reload<=0) continue;
        g.reload=std::max(0.f,g.reload-dt);
        if (g.reload>0) continue;
        const int refill=g.reserve<0?g.capacity-g.ammo:std::min(g.capacity-g.ammo,g.reserve);
        g.ammo+=refill;
        if (g.reserve>=0) g.reserve-=refill;
    }
    auto& p=player;
    if ((!held(Key::fire) && !held(Key::automatic) && p.fireBuffer<=0) || p.fireDelay>0) return;
    p.fireBuffer=0;
    auto& g=guns[gun];
    if (g.reload>0) return;
    if (!g.ammo) {
        reload();
        if (!g.reload) { p.fireDelay=.32f; audio.event("empty"); }
        return;
    }
    const auto& w=weapons[gun];
    const GunSound& sound=gunSounds[gun];
    --g.ammo; p.fireDelay=g.interval; p.muzzle=.069f;
    const Vec exit=muzzle();
    const Vec pivot=weaponPivot();
    float crateDistance=1;
    const bool crateBeforeMuzzle=crateOnSegment(pivot,exit-pivot,crateDistance)>=0;
    const Vec origin=crateBeforeMuzzle?pivot:exit;
    if (!crateBeforeMuzzle && blocked(pivot,exit)) {
        burst(exit,hex(0x9ba8ba),5,96); audio.event(sound.name,.62f*sound.gain,0,sound.rate); return;
    }
    const bool longShot=gun==2 || gun==6;
    for (int i=0;i<w.pellets;++i) {
        const float spread=w.pellets==1?random()-.5f:float(i)/(w.pellets-1)-.5f;
        const bool crit=random()<g.crit;
        Shot shot{origin,ray(p.aim+spread*g.spread)*w.speed,g.range/w.speed,float(g.damage)*(crit?2.f:1.f),hex(w.color),w.pierce,{},longShot,false};
        shot.gravity=w.gravity; shot.blast=w.blast; shot.crit=crit; shot.weapon=gun;
        shot.id=++nextShot;
        shots.push_back(shot);
    }
    const bool heavy=gun==1 || gun==5 || gun==6;
    burst(exit,hex(w.color),gun==1?9:gun==6?10:4,gun==6?220:140);
    burst(exit,hex(0xd9dde2),gun==4?1:heavy?4:2,48,ParticleKind::puff);
    if (gun!=2 && gun!=6 && gun!=5) {
        const Vec eject=weaponPivot()+ray(p.aim)*10.f;
        const float side=p.facing>0?-1.f:1.f;
        const float life=.9f+random()*.5f, angle=random()*2*pi;
        const Vec v=Vec{side*(70+random()*90),-(150+random()*120)};
        particles.push_back({eject,v,gun==1?hex(0xff6a55):hex(0xf2c46b),life,life,1.6f,(random()-.5f)*26,angle,ParticleKind::casing});
    }
    shake=std::max(shake,gun==1?6.8f:gun==6?9.f:gun==5?6.2f:gun==2?4.8f:gun==4?1.7f:2.6f);
    punch=std::max(punch,gun==1?.30f:gun==6?.5f:gun==5?.38f:gun==2?.22f:gun==4?.06f:.10f);
    audio.event(sound.name,.88f*sound.gain,0,sound.rate*(.96f+random()*.074f));
}
} // namespace vl
