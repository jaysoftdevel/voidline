#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::ring(Vec p,Color color,float radius,float life) { rings.push_back({p,color,life,life,radius}); }

void Game::message(Vec p,std::string text,Color color,float duration) {
    messages.push_back({p,std::move(text),color,duration,duration});
}

void Game::burst(Vec p,Color color,int count,float speed,ParticleKind kind) {
    count=std::min(count,420-int(particles.size()));
    for (int i=0;i<count;++i) {
        const bool puff=kind==ParticleKind::puff;
        const float life=puff?.7f+random()*.9f:.18f+random()*(kind==ParticleKind::smoke?.66f:.46f);
        const float angle=random()*2*pi;
        const Vec v=ray(angle)*(speed*(.25f+random()*.75f))-Vec{0,puff?26.f:18.f};
        particles.push_back({p,v,color,life,life,puff?7+random()*9:1+random()*3,(random()-.5f)*(puff?2.f:12.f),angle,kind});
    }
}

// Particles thrown in a cone around a direction. A spread of pi covers a half circle.
void Game::burstCone(Vec p,Vec direction,float spread,Color color,int count,float speed,ParticleKind kind) {
    count=std::min(count,420-int(particles.size()));
    const float base=std::atan2(direction.y,direction.x);
    for (int i=0;i<count;++i) {
        const bool puff=kind==ParticleKind::puff;
        const float life=puff?.5f+random()*.6f:.16f+random()*(kind==ParticleKind::smoke?.5f:.42f);
        const float angle=base+(random()-.5f)*spread;
        const Vec v=ray(angle)*(speed*(.3f+random()*.7f));
        particles.push_back({p,v,color,life,life,puff?5+random()*6:1+random()*2.6f,(random()-.5f)*(puff?2.f:12.f),angle,kind});
    }
}

void Game::updateParticles(float dt) {
    for (auto& p:particles) {
        p.life-=dt;
        p.angle+=p.spin*dt;
        const Vec before=p.p;
        const bool puff=p.kind==ParticleKind::puff;
        const bool smoke=p.kind==ParticleKind::smoke || puff;
        const bool casing=p.kind==ParticleKind::casing || p.kind==ParticleKind::weapon;
        p.v=p.v*(1.f-(smoke?2.6f:casing?.2f:.62f)*dt)+Vec{0,(smoke?-24.f:casing?1100.f:390.f)*dt};
        p.p+=p.v*dt;
        if (p.v.y<=0 || smoke) continue;
        for (const auto& surface:platforms) {
            const auto& s=surface.box;
            if (before.y>s.y || p.p.y<s.y || p.p.x<s.x || p.p.x>s.right()) continue;
            p.p.y=s.y-.1f; p.v.y*=p.kind==ParticleKind::weapon?-.22f:-.34f; p.v.x*=.62f; p.spin*=.55f; break;
        }
    }
    for (auto& r:rings) r.life-=dt;
    if (comboTimer>0 && (comboTimer-=dt)<=0) combo=0;
    style.life=std::max(0.f,style.life-dt);
    for (auto& m:messages) { m.life-=dt; m.p.y-=25*dt; }
    hitMarker=std::max(0.f,hitMarker-dt); killMarker=std::max(0.f,killMarker-dt);
    std::erase_if(particles,[](const Particle& p){return p.life<=0;});
    std::erase_if(messages,[](const Message& m){return m.life<=0;});
    std::erase_if(rings,[](const Ring& r){return r.life<=0;});
}
} // namespace vl
