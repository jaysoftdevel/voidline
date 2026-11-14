#include "render/figures/enemy_view.hpp"

namespace vl {
// Draws one enemy or one corpse. The look of each type is in enemy_*.cpp. This file builds the shared view and
// adds what every enemy has in common: bullet wounds and the health bar.
void Renderer::drawEnemy(const Enemy& e,Color accent,Biome biome,float time,Vec target) {
    EnemyView v(e);
    if (e.elite) { accent=hex(0xffcb68); v.alert=accent; }
    v.accent=accent;
    v.biome=biome; v.facing=e.facing; v.time=time; v.turn=e.turn; v.target=target;
    v.pace=clamp(std::abs(e.vx)/96);
    v.breath=std::sin(time*2.1f+e.phase);
    v.dead=e.alive?0.f:e.deathAge;
    v.burnt=e.alive?0.f:smooth(v.dead/.7f);

    const Color charcoal=hex(0x14171c);
    const Color rim=e.hit>0?hex(0xffffff):mix(hex(0x9fb0b8),accent,.35f);
    const Color hull=e.hit>0?hex(0xfff0d8):mix(hex(0x39424e),accent,.24f);
    const Color plate=e.hit>0?hex(0xffffff):mix(hex(0x56626f),accent,.3f);
    // Corpses lose their color and their lights. Lights stutter on the way out.
    const float flicker=e.alive?1.f:(1-v.burnt)*(std::sin(time*47.f+e.phase)>-.2f?1.f:.15f);
    v.rim=mix(rim,charcoal,v.burnt*.55f);
    v.hull=mix(hull,charcoal,v.burnt*.8f);
    v.plate=mix(plate,charcoal,v.burnt*.7f);
    v.alert=v.alert.energy(flicker);

    // Hits shove the body. A guardian that is dying shudders and sinks.
    Vec shove{e.kick*e.flinch*4.f,0};
    if (e.kind==EnemyKind::boss && !e.alive) {
        const float rise=clamp(v.dead/3.6f);
        shove={std::sin(time*61.f)*(2.f+4.f*rise),std::cos(time*53.f)*2.f+smooth(rise)*16.f};
    }
    v.center=e.box.center()+offset+shove;
    v.x=v.center.x; v.y=v.center.y;
    v.ground={v.x,e.box.bottom()+offset.y};

    switch (e.kind) {
    case EnemyKind::drone: drawDrone(v); break;
    case EnemyKind::turret: drawTurret(v); break;
    case EnemyKind::stalker: drawStalker(v); break;
    case EnemyKind::ranger: drawSoldier(v); break;
    case EnemyKind::boss: drawGuardian(v); break;
    }
    if (e.elite && e.alive) {
        const Vec at{v.x,v.y-e.box.h*.5f-18};
        line(at+Vec{-6,0},at+Vec{0,-4},1.7f,v.alert);
        line(at+Vec{0,-4},at+Vec{6,0},1.7f,v.alert);
    }
    drawWounds(v);
    if (e.warning>0 && e.alive) {
        const Vec start{v.x,v.y-3}, dir=ray(e.aim);
        line(start,start+dir*99,1.1f,v.alert.energy(1.4f).alpha(.34f));
        halo(start,26,v.alert.energy(1.5f).alpha(.54f));
    }
    drawEnemyBar(v);
}

// Bullet holes with a scorched ring and a glowing rim that cools, cracks around the hole and a bright flash on the
// moment of impact. Wounds are stored relative to the body, so they follow it as it moves, falls and tumbles.
void Renderer::drawWounds(const EnemyView& v) {
    const Enemy& e=v.e;
    const int count=std::min<int>(e.woundCount,static_cast<int>(e.wounds.size()));
    for (int i=0;i<count;++i) {
        const Wound& w=e.wounds[static_cast<size_t>(i)];
        const Vec at=v.center+w.local;
        const float s=w.size;
        const float glow=clamp(1-w.age/2.6f)*(1-v.burnt*.6f);
        const Vec dir=ray(w.angle),side{-dir.y,dir.x};
        ellipse(at,{2.9f*s,2.7f*s},v.black.alpha(.45f));
        // Cracks radiating from the hole.
        for (int k=0;k<3;++k) {
            const float a=w.angle+pi*.5f+(k-1)*.9f+(i*.37f);
            const Vec c0=at+ray(a)*2.f*s,c1=at+ray(a)*(4.2f+k*.8f)*s;
            line(c0,c1,.7f,v.black.alpha(.55f));
        }
        ellipse(at,{1.8f*s,1.6f*s},v.black);
        if (glow>.02f) {
            line(at-side*1.9f*s,at+side*1.9f*s,.8f,hex(0xff8a3a).energy(1.6f*glow).alpha(.85f*glow));
            halo(at,6.f*s,hex(0xff8a3a).alpha(.28f*glow));
        }
        // Impact flash, brief and white.
        if (w.age<.14f) {
            const float f=1-w.age/.14f;
            halo(at,16.f*s*(.6f+.4f*f),hex(0xffffff).energy(2.f).alpha(.7f*f));
            line(at-dir*5.f*s,at+dir*5.f*s,1.4f,hex(0xfff0c8).energy(2.4f).alpha(f));
        }
    }
}

// Health bar for the few seconds after a hit. A pale segment trails behind the red one and drains slowly,
// so the size of each hit is visible.
void Renderer::drawEnemyBar(const EnemyView& v) {
    const Enemy& e=v.e;
    if (!e.alive || e.kind==EnemyKind::boss || e.bar<=0) return;
    const float fade=clamp(e.bar/.5f);
    const float fraction=clamp(float(e.hp)/e.maxHp);
    const float span=e.kind==EnemyKind::drone?26.f:e.kind==EnemyKind::ranger?30.f:34.f;
    const Rect track{v.x-span*.5f,v.y-e.box.h*.5f-12,span,3.6f};
    rect({track.x-1.2f,track.y-1.2f,track.w+2.4f,track.h+2.4f},v.black.alpha(.85f*fade));
    rect(track,hex(0x2a2f38).alpha(fade));
    rect({track.x,track.y,track.w*clamp(e.trail),track.h},hex(0xfff2d0).alpha(.95f*fade));
    rect({track.x,track.y,track.w*fraction,track.h},mix(hex(0xff5a4a),hex(0xff9a5a),fraction).alpha(fade));
    // Tick marks every quarter.
    for (int k=1;k<4;++k) rect({track.x+track.w*k*.25f-.3f,track.y,.6f,track.h},v.black.alpha(.6f*fade));
}
} // namespace vl
