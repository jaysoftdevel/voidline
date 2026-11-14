#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::ambient(const Game& g) {
    const Color accent=hex(g.stage->accent),cold=hex(g.stage->cold);
    if (g.skyFlash>.01f) rect({0,0,width,height},g.skyColor.alpha(std::min(1.f,g.skyFlash)*.085f));
    for (const auto& a:g.ambient) {
        const Vec c{a.p.x+offset.x*a.depth,a.p.y+offset.y*a.depth};
        if (c.x<-340 || c.x>width+340) continue;
        const float age=a.maxLife-a.life;
        const float fade=smooth(std::min({age/.5f,a.life/.6f,1.f}));
        const float t=age/a.maxLife;
        const float s=a.size;
        switch (a.kind) {
        case AmbientKind::blast: {
            const float flash=a.pulse();
            const Color fire=a.variant==1?hex(0xff8a3c):accent;
            halo(c,(110+t*150)*s,fire.energy(2.4f).alpha(.55f*flash));
            // Fireball: hot cloud sprites that cool from white to orange to soot while they rise.
            for (int i=0;i<9;++i) {
                const float jitter=hashf(a.seed,i);
                const Vec at=c+Vec{(jitter-.5f)*56*s*(.4f+t),-(t*46+jitter*22)*s+(hashf(a.seed,i+9)-.5f)*24*s};
                const float heat=clamp(1-t*1.9f-jitter*.25f);
                const Color tint=mix(hex(0x2a2f38),mix(fire.energy(1.6f),hex(0xfff1cf).energy(2.4f),heat*heat),heat);
                puff(at,(16+t*46+jitter*12)*s,i*1.3f+t*1.5f,tint.alpha(clamp((1-t*.8f))*(.55f+.3f*heat)));
            }
            ellipse(c,{(8+t*26)*s*flash,(8+t*26)*s*flash},hex(0xfff2d0).energy(3.4f).alpha(flash));
            // Sparks and debris on ballistic arcs.
            for (int i=0;i<14;++i) {
                const float angle=-pi*(.08f+.84f*hashf(a.seed,i+3)),speed=90+hashf(a.seed,i+30)*210;
                const Vec at=c+Vec{std::cos(angle)*speed*t*1.5f,std::sin(angle)*speed*t*1.5f+210*t*t}*s;
                line(at,at+ray(angle+.5f+t)*5.f*s,1.5f,fire.energy(2.2f).alpha((1-t)*.9f));
            }
            // A faint low shockwave along the ground plane.
            if (t<.5f) {
                const float w=(10+t*420)*s;
                beam(c+Vec{-w,34*s},3,c+Vec{w,34*s},3,hex(0xffffff).energy(1.4f).alpha(.10f*(1-t*2)),hex(0xffffff).energy(1.4f).alpha(.10f*(1-t*2)));
            }
            break;
        }
        case AmbientKind::flyer: {
            const float dir=a.v.x>0?1.f:-1.f;
            if (a.variant==0) {
                // Silhouetted transport with strobing nav lights and a glowing engine.
                const float big=(g.stage->biome==Biome::orbital?1.55f:1.f)*s;
                const Color hull=hex(0x070b12);
                const Vec nose=c+Vec{dir*34*big,0}, tail=c-Vec{dir*34*big,0};
                polygon(nose,c+Vec{dir*6*big,-8*big},tail+Vec{dir*4*big,-6*big},c+Vec{0,7*big},hull.alpha(.94f*fade));
                polygon(c+Vec{dir*4*big,-6*big},c-Vec{dir*14*big,0}+Vec{0,-21*big},c-Vec{dir*22*big,6*big},c-Vec{dir*8*big,-2*big},hull.alpha(.94f*fade));
                polygon(tail+Vec{dir*2*big,-6*big},tail-Vec{dir*8*big,16*big},tail-Vec{dir*14*big,8*big},tail,hull.alpha(.94f*fade));
                const float blink=std::fmod(age*1.7f+a.seed,1.f)<.12f?1.f:.18f;
                halo(c+Vec{dir*5*big,-7*big},14*big,hex(0xff4040).energy(2.f).alpha(.55f*fade*blink));
                halo(c-Vec{dir*8*big,-3*big},14*big,hex(0x60ff90).energy(2.f).alpha(.45f*fade));
                ellipse(c+Vec{dir*20*big,3.f*big},{2.f*big,1.5f*big},hex(0xfff0d0).energy(2.4f).alpha(fade));
                halo(tail-Vec{dir*3*big,0},34*big,accent.energy(2.2f).alpha(.50f*fade));
                beam(tail-Vec{dir*3*big,0},9*big,tail-Vec{dir*74*big,0},2*big,accent.energy(2.5f).alpha(.50f*fade),accent.alpha(0));
                // Cabin windows.
                for (int i=0;i<4;++i) rect({c.x-dir*(i*6.f-6.f)*big-1.f,c.y-2.f*big,2.2f*big,1.6f*big},hex(0xffd9a0).energy(1.6f).alpha(.8f*fade));
            } else if (a.variant==1) {
                // A flock of bats or birds with flapping wings.
                for (int i=0;i<9;++i) {
                    const Vec at=c+Vec{-dir*(hashf(a.seed,i)*95.f+(i%3)*8.f),std::sin(age*1.7f+i)*14.f+(hashf(a.seed,i+20)-.5f)*60.f};
                    const float flap=std::sin(age*(13.f+hashf(a.seed,i+9)*4.f)+i*1.7f);
                    const float span=(8.f+hashf(a.seed,i+40)*4.f)*s;
                    const Color wing=hex(0x06080d).alpha(.88f*fade);
                    line(at,at+Vec{-span,-flap*span*.7f},2.f,wing);
                    line(at,at+Vec{span,-flap*span*.7f},2.f,wing);
                    ellipse(at,{2.4f*s,1.8f*s},wing);
                }
            } else {
                // A luminous eel gliding under the water line.
                for (int i=0;i<12;++i) {
                    const float u=i/11.f;
                    const Vec at=c-Vec{dir*u*74*s,-std::sin(age*3.6f-u*5.f)*(3.f+u*5.f)*s};
                    const float w=(1-u*.7f)*5.f*s;
                    ellipse(at,{w,w*.8f},cold.energy(1.6f-u*.6f).alpha(.60f*fade*(1-u*.5f)));
                    halo(at,w*5.f,cold.alpha(.24f*fade*(1-u*.6f)));
                }
                halo(c,20*s,hex(0xe6ffff).alpha(.45f*fade));
            }
            break;
        }
        case AmbientKind::fountain: {
            const Color tint=a.variant==1?hex(0xcdf8ff):a.variant==2?hex(0xffa04a):a.variant==3?hex(0xb7ff9d):accent;
            const float pulse=std::sin(clamp(t*1.4f)*pi);
            if (a.variant==2 || a.variant==0) halo(c,60*s,tint.energy(2.f).alpha(.40f*pulse));
            if (a.variant==1) for (int i=0;i<4;++i) puff(c+Vec{(i-1.5f)*12,-t*20-i*6},(20+i*6)*s,i*1.7f,hex(0xdcf2f6).alpha(.20f*pulse));
            for (int i=0;i<30;++i) {
                const float delay=hashf(a.seed,i)*.45f*a.maxLife;
                const float tt=age-delay;
                if (tt<0 || tt>1.7f) continue;
                const float angle=-pi*.5f+(hashf(a.seed,i+50)-.5f)*(a.variant==1?.55f:.95f);
                const float speed=(a.variant==1?300.f:230.f+hashf(a.seed,i+90)*220.f)*s;
                const Vec vel=ray(angle)*speed;
                const Vec at=c+Vec{vel.x*tt,vel.y*tt+.5f*520.f*tt*tt};
                const Vec tail=normal(Vec{vel.x,vel.y+520.f*tt})*(a.variant==2?9.f:6.f);
                const float alpha=clamp(1.7f-tt)*pulse*.95f+.05f;
                line(at-tail,at,a.variant==2?2.6f:1.8f,tint.energy(2.2f).alpha(alpha));
                if (a.variant==2) halo(at,11,tint.energy(1.8f).alpha(.20f*alpha));
            }
            break;
        }
        case AmbientKind::streak: {
            const Vec dirv=normal(a.v);
            const float alpha=std::min(1.f,std::sin(clamp(t)*pi)*2.f);
            const Color tint=g.stage->biome==Biome::biodome?hex(0xd6b8ff):hex(0xfff0d8);
            beam(c,2,c-dirv*86,.4f,tint.energy(2.4f).alpha(.9f*alpha),tint.alpha(0));
            halo(c,10,tint.energy(2.4f).alpha(.7f*alpha));
            break;
        }
        case AmbientKind::lightning: {
            const float pulse=a.pulse();
            if (pulse<.03f) break;
            const Color bolt=hex(0xdcecff);
            halo(Vec{c.x,c.y-30},260,bolt.energy(1.8f).alpha(.35f*std::min(1.f,pulse)));
            float x=c.x,y=-10;
            const int segments=15;
            for (int i=0;i<segments;++i) {
                const float nx=x+(hashf(a.seed,i)-.5f)*46,ny=y+(c.y+10)/segments;
                const float p=std::min(1.f,pulse);
                line({x,y},{nx,ny},5.f,bolt.energy(2.f).alpha(.30f*p));
                line({x,y},{nx,ny},1.8f,hex(0xffffff).energy(2.6f).alpha(.95f*p));
                if (i%4==2) {
                    const float bx=nx+(hashf(a.seed,i+30)-.5f)*120,by=ny+60;
                    line({nx,ny},{bx,by},1.2f,bolt.energy(2.f).alpha(.7f*p));
                }
                x=nx; y=ny;
            }
            break;
        }
        case AmbientKind::searchlight: {
            const float sweep=std::sin(age*.8f)*.58f*(a.v.y>0?1.f:-1.f);
            const float angle=a.v.x+sweep;
            const Vec dirv=ray(angle);
            const Color tint=g.stage->biome==Biome::orbital?hex(0xb7d4ff):hex(0xfff0cc);
            beam(c,4,c+dirv*980,190,tint.energy(1.6f).alpha(.26f*fade),tint.alpha(0));
            beam(c,2,c+dirv*980,70,tint.energy(2.f).alpha(.14f*fade),tint.alpha(0));
            halo(c+dirv*10,26,tint.energy(2.f).alpha(.4f*fade));
            break;
        }
        case AmbientKind::steam: {
            const bool hot=a.variant==1;
            for (int i=0;i<9;++i) {
                const float u=std::fmod(age*.55f+i*.11f,1.f);
                const Vec at=c+Vec{std::sin(i*1.9f)*9*s+u*(hot?26.f:16.f)*s,-u*150*s};
                puff(at,(12+u*50)*s,i*.9f+age*.3f,(hot?hex(0xe0a26a):hex(0xdcecf0)).alpha(.32f*(1-u)*fade));
            }
            if (t<.45f) {
                const float jet=1-t/.45f;
                beam(c,3,c+Vec{16,-86}*s,16,hex(0xffffff).energy(1.6f).alpha(.5f*jet),hex(0xffffff).alpha(0));
            }
            break;
        }
        case AmbientKind::spores: {
            const Color glow=g.stage->biome==Biome::biodome?hex(0xd18cff):cold;
            for (int i=0;i<24;++i) {
                const float hx=hashf(a.seed,i);
                const Vec at=c+Vec{std::sin(hx*6.f+age*.7f)*55*s+(hx-.5f)*150*s,-age*(14+hashf(a.seed,i+10)*24)+std::sin(age*2.f+i)*6};
                const float twinkle=.55f+.45f*std::sin(age*3.f+i*1.3f);
                halo(at,(6+hashf(a.seed,i+20)*7)*s,glow.energy(1.8f).alpha(.45f*fade*twinkle));
                rect({at.x-.8f,at.y-.8f,1.6f,1.6f},hex(0xffffff).energy(2.f).alpha(.9f*fade*twinkle));
            }
            break;
        }
        }
    }
}
} // namespace vl
