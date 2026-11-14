#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::drawParticles(const Game& g) {
    const Color cold=hex(g.stage->cold),accent=hex(g.stage->accent);
    for (int i=0;i<66;++i) {
        const float depth=hash(i+190);
        float x=std::fmod(hash(i*7+14)*1320+g.time*(6+depth*17)+offset.x*(.08f+depth*.13f),1320.f);
        if (x<0) x+=1320;
        const float y=120+hash(i+80)*475+std::sin(g.time*(.5f+depth)+i)*15;
        const float glow=.16f+depth*.26f;
        const Vec p{x-60,y};
        const Color tint=g.stage->biome==Biome::forge?accent:cold;
        if (i%4==0) halo(p,3+depth*4,tint.alpha(glow));
        rect({p.x,p.y,.6f+depth*1.9f,.7f+depth*1.6f},tint.energy(1.23f).alpha(glow));
    }
    for (const auto& particle:g.particles) {
        if (!visible(particle.p.x)) continue;
        const float life=particle.life/particle.maxLife;
        const Vec p=particle.p+offset;
        const Color tint=particle.color;
        if (particle.kind==ParticleKind::stain) {
            ellipse(p,{particle.size*(1.25f+0.3f*(1-life)),particle.size*.38f},
                    hex(0x22040c).alpha(.72f*life));
            ellipse(p,{particle.size*.80f,particle.size*.20f},hex(0x9b1e2b).alpha(.56f*life));
        } else if (particle.kind==ParticleKind::blood) {
            const Vec trail=normal(particle.v)*std::max(1.5f,particle.size*1.5f);
            line(p-trail,p,particle.size*.75f,hex(0x3a0610).alpha(life));
            ellipse(p,{particle.size*.64f,particle.size*.64f},tint.alpha(life));
        } else if (particle.kind==ParticleKind::smoke) {
            halo(p,particle.size*(4+9*(1-life)),tint.alpha(life*.22f));
        } else if (particle.kind==ParticleKind::puff) {
            puff(p,particle.size*(1.4f+3.2f*(1-life)),particle.angle,tint.alpha(life*.34f));
        } else if (particle.kind==ParticleKind::weapon) {
            // A dropped rifle tumbling across the floor.
            const Vec d=ray(particle.angle);
            const Vec n{-d.y,d.x};
            const float fade=clamp(particle.life*2.f);
            line(p-d*12.f,p+d*12.f,4.4f,hex(0x06080d).alpha(fade));
            line(p-d*11.f,p+d*10.f,2.6f,hex(0x56626f).alpha(fade));
            line(p+d*3.f,p+d*12.f,1.4f,hex(0x93a5ac).alpha(fade));
            line(p-d*1.f+n*1.5f,p-d*1.f+n*5.5f,2.f,hex(0x06080d).alpha(fade));
        } else if (particle.kind==ParticleKind::casing) {
            const Vec d=ray(particle.angle)*2.5f;
            line(p-d,p+d,2.6f,hex(0x1a1208).alpha(clamp(particle.life*3)));
            line(p-d,p+d,1.5f,tint.energy(1.25f).alpha(clamp(particle.life*3)));
        } else if (particle.kind==ParticleKind::chip) {
            const Vec d=ray(particle.angle)*particle.size;
            const Vec o{-d.y*.50f,d.x*.50f};
            triangle(p+d,p-d+o,p-d-o,tint.energy(.88f+life*.67f).alpha(life));
        } else {
            const Vec v=normal(particle.v)*particle.size*(1.9f+life*2);
            line(p-v,p,particle.size*.72f,tint.energy(1.55f+life*1.51f).alpha(clamp(life*2)));
        }
    }
}

void Renderer::drawRings(const Game& g) {
    for (const auto& r:g.rings) {
        if (!visible(r.p.x,r.radius)) continue;
        const float t=1-r.life/r.maxLife;
        const float radius=r.radius*(1-(1-t)*(1-t));
        const float alpha=(1-t)*(1-t);
        const Vec c=r.p+offset;
        constexpr int segments=28;
        for (int i=0;i<segments;++i) {
            const Vec a=c+ray(i*2*pi/segments)*radius, b=c+ray((i+1)*2*pi/segments)*radius;
            line(a,b,1.5f+7.f*(1-t),r.color.energy(2.f).alpha(alpha*.85f));
            line(a,b,1.f+2.f*(1-t),hex(0xffffff).alpha(alpha));
        }
        halo(c,radius*1.25f,r.color.energy(1.6f).alpha(alpha*.35f));
    }
}

void Renderer::drawTrails(const Game& g,const Operative& op,Vec ground) {
    const auto& p=op.player;
    const float speed=std::abs(p.velocity.x);
    const float slow=clamp((1-g.timeScale)/.72f);
    const bool flipping=p.flip>0;
    const float intensity=std::max(clamp((speed-300)/220),flipping?1.f:0.f);
    if (intensity<=.02f && slow<.1f) return;
    const Color tint=hex(op.character->color);
    const Vec travel=length(p.velocity)>1?normal(p.velocity):Vec{float(p.facing),0};
    const Vec center=ground-Vec{0,26};
    const Vec perpendicular{-travel.y,travel.x};
    for (int i=0;i<4;++i) {
        const float lane=(i-1.5f)*9.f;
        const float streak=18+hash(i+int(g.time*30))*26+intensity*34+slow*20;
        const Vec start=center+perpendicular*lane-travel*(6+hash(i+9)*8);
        line(start,start-travel*streak,1.6f+hash(i+40)*1.4f,tint.energy(1.5f).alpha((.12f+.26f*std::max(intensity,slow))*(1-std::abs(lane)/24)));
    }
}
} // namespace vl
