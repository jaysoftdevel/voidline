#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// Turns a freshly killed enemy into a corpse and runs the shared part of every death animation.
// The kind-specific motion lives in death_soldier.cpp, death_machines.cpp and death_guardian.cpp.
void Game::bury(const Enemy& e) {
    const float away=e.fallDir;
    Corpse c;
    c.body=e;
    c.body.alive=false;
    c.body.hit=.12f;
    c.body.deathAge=0;
    const float strength=clamp(length(e.hitDir)*0+.5f+std::abs(e.kick)*.5f,.4f,1.f);
    switch (e.kind) {
    case EnemyKind::ranger:
        // The soldier staggers back, buckles and topples about its feet, dropping the rifle.
        c.pivotFeet=true; c.rest=away*1.52f; c.life=3.2f;
        c.v={away*(55+strength*150),0};
        break;
    case EnemyKind::drone:
        c.v={away*(70+random()*90),-60}; c.spin=away*(3+random()*4); c.rest=away*.5f; c.life=2.4f;
        break;
    case EnemyKind::turret:
        c.life=2.2f;
        break;
    case EnemyKind::stalker:
        c.v={away*(110+random()*110),-(190+random()*120)}; c.spin=away*(5+random()*3); c.rest=away*pi; c.life=2.4f;
        break;
    case EnemyKind::boss:
        c.life=3.6f; c.nextBlast=.15f;
        break;
    }
    corpses.push_back(c);
    if (e.kind==EnemyKind::ranger) {
        // The rifle leaves the hands and tumbles to the ground.
        const Vec hand=e.box.center()+Vec{e.fallDir*-6.f,-2.f};
        const float life=3.f;
        particles.push_back({hand,{away*(40+random()*50),-(120+random()*90)},hex(0x56626f),life,life,1,(random()-.5f)*9.f,random()*2*pi,ParticleKind::weapon});
    }
}

// Falls under gravity and lands on the first platform below. Returns true on the step it touches down.
bool Game::dropCorpse(Corpse& c,float dt,float pull) {
    Vec center=c.body.box.center();
    c.v.y+=pull*dt;
    c.v.x*=1-.9f*dt;
    const Vec before=center;
    center+=c.v*dt;
    const float half=c.body.box.h*.5f*(1-.3f*std::abs(std::sin(c.angle)));
    bool landed=false;
    for (const auto& surface:platforms) {
        const auto& s=surface.box;
        if (c.v.y<=0 || center.x<s.x || center.x>s.right()) continue;
        const float prior=before.y+half;
        if (prior>s.y+3 || center.y+half<s.y) continue;
        center.y=s.y-half;
        landed=true;
        break;
    }
    c.body.box.x=center.x-c.body.box.w*.5f; c.body.box.y=center.y-c.body.box.h*.5f;
    return landed;
}

void Game::updateCorpses(float dt) {
    for (auto& c:corpses) {
        c.age+=dt;
        c.body.deathAge=c.age;
        c.body.hit=std::max(0.f,c.body.hit-dt);
        c.body.flinch=std::max(0.f,c.body.flinch-dt*3.f);
        c.body.headSnap=std::max(0.f,c.body.headSnap-dt*3.f);
        // Wounds keep glowing and cooling on the body.
        for (auto& w:c.body.wounds) w.age+=dt;
        switch (c.body.kind) {
        case EnemyKind::ranger: updateSoldierCorpse(c,dt); break;
        case EnemyKind::drone: updateDroneCorpse(c,dt); break;
        case EnemyKind::turret: updateTurretCorpse(c,dt); break;
        case EnemyKind::stalker: updateStalkerCorpse(c,dt); break;
        case EnemyKind::boss: updateGuardianCorpse(c,dt); break;
        }
    }
    // A last puff of smoke when a corpse is removed.
    for (const auto& c:corpses)
        if (c.age>=c.life && c.body.kind!=EnemyKind::boss) burst(c.body.box.center(),hex(0x98b4c7),6,80,ParticleKind::smoke);
    std::erase_if(corpses,[](const Corpse& c){return c.age>=c.life;});
}
} // namespace vl
