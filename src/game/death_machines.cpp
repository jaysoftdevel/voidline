#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// A secondary explosion on a wreck: fire, a smoke puff, a ring and a muffled bang.
void Game::wreckBlast(Vec at,float size) {
    const Color accent=hex(stage->accent);
    burst(at,accent,int(10+14*size),180+120*size);
    burst(at,hex(0xfff0b8),int(5+8*size),260+160*size);
    burst(at,hex(0x6d747c),int(2+3*size),60,ParticleKind::puff);
    burst(at,hex(0x93a5ac),int(4+6*size),180,ParticleKind::chip);
    ring(at,hex(0xfff0c2),34+60*size,.4f);
    shake=std::max(shake,3.f+7.f*size);
    audio.event("destroy",.35f+.4f*size,(at.x-player.box.x)/width,.7f+random()*.2f);
}

// A drone spins out of the air trailing fire, loses a rotor and blows up when it hits the floor.
void Game::updateDroneCorpse(Corpse& c,float dt) {
    if (c.settled) { 
        if (random()<dt*10) burst(c.body.box.center(),hex(0x4b4f56),1,22,ParticleKind::puff);
        return;
    }
    const bool landed=dropCorpse(c,dt,900.f);
    c.angle+=c.spin*dt;
    const Vec center=c.body.box.center();
    if (random()<dt*28) burst(center,hex(0xffa55e),1,50);
    if (random()<dt*14) burst(center,hex(0x3a3f46),1,28,ParticleKind::puff);
    if (c.blasts==0 && c.age>.18f) {
        // A rotor shears off and spins away on its own.
        c.blasts=1;
        const float side=c.v.x>=0?1.f:-1.f;
        for (int i=0;i<2;++i)
            particles.push_back({center+Vec{side*(i?-18.f:18.f),-6},{side*(120+random()*120)*(i?-1.f:1.f),-(80+random()*100)},hex(0x93a5ac),1.8f,1.8f,2.4f,(random()-.5f)*26.f,random()*2*pi,ParticleKind::chip});
        burstCone(center,{side,-.3f},1.2f,hex(0xffd9a0),8,260);
    }
    if (landed || c.age>1.6f) {
        c.settled=true; c.scorched=true; c.v={};
        wreckBlast(center,.8f);
        // The airframe is left charred where it came to rest.
        c.angle=c.rest; c.life=std::min(c.life,c.age+1.2f);
    }
}

// A turret sags on its mount, sparks and smokes, then a secondary explosion throws its barrels off.
void Game::updateTurretCorpse(Corpse& c,float dt) {
    const Vec center=c.body.box.center();
    if (c.age<.55f) {
        if (random()<dt*34) burst(center+Vec{(random()-.5f)*22.f,-6},hex(0xffd27a),1,130);
        if (random()<dt*16) burst(center,hex(0x3a3f46),1,32,ParticleKind::puff);
    } else if (c.blasts==0) {
        c.blasts=1; c.scorched=true;
        wreckBlast(center,.7f);
        for (int i=0;i<2;++i)
            particles.push_back({center+Vec{(i?-1.f:1.f)*6.f,-8.f},{(i?-1.f:1.f)*(90+random()*120),-(200+random()*130)},hex(0x93a5ac),2.f,2.f,2.6f,(random()-.5f)*22.f,random()*2*pi,ParticleKind::chip});
    } else if (random()<dt*9) {
        burst(center+Vec{0,-8},hex(0x4b4f56),1,22,ParticleKind::puff);
    }
}

// A stalker is flung onto its back, its legs twitch and curl in, then it stops moving.
void Game::updateStalkerCorpse(Corpse& c,float dt) {
    if (!c.settled) {
        const bool landed=dropCorpse(c,dt,1500.f);
        c.angle+=c.spin*dt;
        if (std::abs(c.angle)>std::abs(c.rest)) { c.angle=c.rest; c.spin*=.2f; }
        if (random()<dt*18) burst(c.body.box.center(),hex(0xffd27a),1,90);
        if (landed) {
            if (c.v.y>260) {
                c.v.y*=-.28f; c.v.x*=.6f;
                burst({c.body.box.center().x,c.body.box.bottom()},hex(stage->cold),4,70,ParticleKind::smoke);
                audio.event("land",.4f,(c.body.box.center().x-player.box.x)/width,.7f);
            } else { c.v={}; c.settled=true; ring({c.body.box.center().x,c.body.box.bottom()},hex(0xdde5ea),20,.25f); }
        }
    } else {
        c.angle=approach(c.angle,c.rest,dt*4);
        if (random()<dt*8) burst(c.body.box.center(),hex(0xffb36a),1,70);
    }
}
} // namespace vl
