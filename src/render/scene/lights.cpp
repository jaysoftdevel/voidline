#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::collectLights(const Game& g) {
    lights.clear(); blockers.clear();
    const Color cold=hex(g.stage->cold), accent=hex(g.stage->accent);
    for (size_t i=0;i<g.platforms.size();++i) {
        const auto b=platformBox(g,i);
        if (visible(b.x,b.w)) blockers.push_back({b.x+offset.x,b.y+offset.y,b.w,b.h});
    }
    for (size_t i=0;i<g.enemies.size();++i) {
        const auto& e=g.enemies[i];
        const auto b=enemyBox(g,i);
        if (!e.alive || !visible(b.x,b.w)) continue;
        if (e.kind==EnemyKind::boss || e.kind==EnemyKind::turret)
            blockers.push_back({b.x+offset.x+b.w*.18f,b.y+offset.y+b.h*.20f,b.w*.64f,b.h*.70f});
    }
    const auto& p=g.player.box;
    blockers.push_back({p.x+offset.x+8,p.y+offset.y+5,16,p.h-13});
    if (g.coop) {
        const auto& partner=g.teammate();
        const auto& body=partner.player;
        if (visible(body.box.x,body.box.w)) {
            const Vec at=body.box.center()+offset;
            lights.push_back({at,106,.88f,hex(partner.character->color),ray(body.aim),.45f,false});
            if (body.muzzle>0)
                lights.push_back({g.muzzle(partner)+offset,152,3.6f*body.muzzle/.069f,
                                  hex(weapons[partner.gun].color),ray(body.aim),-.1f,true});
        }
    }
    if (g.player.muzzle>0)
        lights.push_back({g.muzzle()+offset+playerShift,178,4.8f*g.player.muzzle/.069f,hex(weapons[g.gun].color),ray(g.player.aim),-.1f,true});
    for (size_t i=0;i<g.enemies.size();++i) {
        const auto& e=g.enemies[i];
        if (!e.alive || (e.kind!=EnemyKind::boss && e.warning<=0)) continue;
        const auto b=enemyBox(g,i);
        if (visible(b.x,b.w)) lights.push_back({b.center()+offset,110,e.warning>0?2.16f:1.35f,accent});
    }
    lights.push_back({{p.center().x+offset.x,p.y+offset.y+std::min(p.h*.31f,16.f)},128,1.05f,hex(g.character->color),ray(g.player.aim),.45f,false});
    for (int i=0;i<static_cast<int>(g.stage->beacons.size());++i) {
        const auto& at=g.stage->beacons[i];
        if (visible(at.x)) lights.push_back({at+offset-Vec{0,47},102,1.28f,i==g.activeBeacon?hex(0x96ffc3):cold});
    }
    if (visible(g.stage->exit.x)) lights.push_back({g.stage->exit+offset-Vec{0,86},165,1.65f,g.bossDead?hex(0x8effc8):hex(0xff7250)});
    for (const auto& vent:g.stage->vents) if (visible(vent.x) && g.ventState(vent.phase)>0)
        lights.push_back({Vec{vent.x,vent.y-7}+offset,98,1.78f,accent,{0,-1},.12f});
    for (int i=0;140+i*273<g.stage->worldWidth+215;i+=2) {
        const float x=140+i*273+hash(i+g.index*32)*90;
        if (!visible(x)) continue;
        const float y=253+hash(i+54)*126;
        lights.push_back({Vec{x+11,y+8}+offset,215,1.86f+.17f*std::sin(g.time*2+i),accent,{.12f,1},.16f});
    }
    for (int i=0;167+511*i<g.stage->worldWidth+215;++i) {
        const float x=167+511*i;
        if (!visible(x)) continue;
        lights.push_back({Vec{x,143+hash(i+43)*102}+offset,301,1.49f,cold,{-.19f,1},.27f});
    }
    for (const auto& s:g.shots) if (s.longShot || s.blast>0) {
        const auto at=shotPosition(s,true);
        if (visible(at.x)) lights.push_back({at+offset,s.blast>0?96.f:58.f,s.blast>0?1.5f:.88f,s.color,{0,1},-1,false});
    }
    for (const auto& a:g.ambient) {
        const float pulse=a.pulse();
        if (pulse<.05f) continue;
        const Vec at{a.p.x+offset.x*a.depth,a.p.y+offset.y*a.depth};
        if (at.x<-300 || at.x>width+300) continue;
        lights.push_back({at,a.kind==AmbientKind::lightning?520.f:380.f,1.25f*std::min(1.f,pulse),g.skyColor,{0,1},-1,false});
    }
    for (const auto& crate:g.crates) if (!crate.spent && visible(crate.box.x,crate.box.w))
        lights.push_back({crate.box.center()+offset,58,.72f,hex(crateSpec(crate.kind).color),{0,1},-1,false});
    for (const auto& s:g.hostile) {
        const auto at=shotPosition(s,false);
        if (visible(at.x)) lights.push_back({at+offset,36,.65f,s.color,{0,1},-1,false});
    }
}

// Sums the light that reaches a point, ignoring occlusion, so figures pick up the color of nearby lamps and flashes.
Color Renderer::litBy(Vec p,const Game& g) const {
    const Color cold=hex(g.stage->cold);
    Color result{.58f+cold.r*.10f,.60f+cold.g*.10f,.66f+cold.b*.10f,1};
    for (const auto& l:lights) {
        if (l.radius<50) continue;
        const Vec d=p-l.p;
        const float r=length(d)/l.radius;
        if (r>=1) continue;
        float spot=1;
        if (l.cone>=0) spot=smooth((dot(normal(d),normal(l.direction))-l.cone)/.29f);
        const float falloff=(1-r*r)*(1-r*r)/(1+5.8f*r*r);
        const float k=falloff*l.energy*spot*.42f;
        result.r+=l.tint.r*k; result.g+=l.tint.g*k; result.b+=l.tint.b*k;
    }
    result.r=std::min(result.r,1.55f); result.g=std::min(result.g,1.55f); result.b=std::min(result.b,1.55f);
    return result;
}
} // namespace vl
