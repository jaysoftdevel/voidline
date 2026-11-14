#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::hud() {
    const auto& p=game.player;
    const bool slow=game.slowMotion() || (game.coop && game.timeScale<.88f);
    const auto& g=game.guns[game.gun];
    const auto& w=weapons[game.gun];
    const Color trim=hex(game.character->color);
    const Color accent=hex(game.stage->accent);
    const Color ammo=hex(w.color);
    const Color ink=hex(0x070a10);
    constexpr float slant=12;
    // Angled panels with a bold colored edge, in the spirit of comic-book frames.
    auto panel=[&](Rect b,Color edge) {
        r.slab({b.x-3,b.y+3,b.w,b.h},slant,ink.alpha(.55f),ink.alpha(.55f));
        r.slab(b,slant,hex(0x101a28).alpha(.94f),hex(0x0a111b).alpha(.92f));
        r.slabOutline(b,slant,edge.alpha(.92f),2.2f);
    };
    panel({19,16,271,75},trim);
    label({31,36},game.character->name,trim);
    r.text({31,59},"SUIT  "+std::to_string(p.hp)+" / "+std::to_string(game.maxHealth),16,white,true);
    for (int i=0;i<10;++i) {
        const Rect cell{31.f+i*24.7f,70,20.7f,7};
        r.slab(cell,3,hex(0x2a3640),hex(0x2a3640));
        const float health=clamp(float(p.hp)*10/game.maxHealth-i);
        if (health>0) r.slab({cell.x,cell.y,cell.w*health,cell.h},3,p.hp<30?hex(0xff5b45):trim,p.hp<30?hex(0xff5b45):trim);
    }
    // Slow-motion reserve.
    {
        const Rect track{22,98,190,9};
        const bool active=slow;
        const Color tint=game.slowEnergy<.22f?hex(0xff8a6a):hex(0x7ff2ff);
        r.slab(track,6,ink.alpha(.8f),ink.alpha(.8f));
        r.slab({track.x,track.y,track.w*game.slowEnergy,track.h},6,tint.energy(active?1.4f:1.f),tint.energy(.7f));
        r.slabOutline(track,6,tint.alpha(.8f),1.2f);
        label({224,107},active?"BULLET TIME":"FOCUS  [Q]",active?tint:muted);
    }
    panel({909,16,272,75},ammo);
    label({921,36},w.name,ammo);
    r.text({922,69},digits(g.ammo),30,white,true);
    r.text({967,67},"/ "+(g.reserve<0?std::string("INF"):std::to_string(g.reserve)),18,pale);
    label({1086,38},"SCORE",muted);
    r.text({1086,67},std::to_string(game.score),24,white,true);
    if (g.reload>0) {
        r.rect({921,77,246,4},hex(0x3e4f5b));
        r.rect({921,77,246*(1-g.reload/g.duration),4},ammo);
    }
    const std::string objective=game.bossDead?(game.coop?"Extract together":"Reach the extraction gate"):game.bossActive?"Defeat "+std::string(game.stage->boss):"Find the sector guardian";
    panel({413,16,374,47},accent);
    label({425,35},"SECTOR "+digits(game.index+1)+"/"+digits(sectorCount),accent);
    r.text({540,37},objective,14,white,true);
    r.rect({425,48,350,3},hex(0x465966));
    r.rect({425,48,350*clamp(p.box.center().x/game.stage->worldWidth),3},accent);
    if (game.bossActive) {
        const auto& boss=game.enemies.back();
        panel({390,72,420,44},hex(0xff8066));
        label({402,91},game.stage->boss,hex(0xff8b71));
        r.rect({402,100,396,6},hex(0x4b333c));
        r.rect({402,100,396*clamp(float(boss.hp)/boss.maxHp),6},hex(0xff8066));
    }
    panel({19,548,282,34},muted);
    const bool covered=p.crouched && game.nearCover();
    label({31,570},covered?"IN COVER":p.crouched?"CROUCH":p.sprinting?"SPRINT":p.flip>0?"FLIP":game.activeBeacon>=0?"CHECKPOINT ONLINE":"FIELD COMMS  /  ACTIVE",
        covered?teal:p.crouched||p.sprinting||p.flip>0?trim:game.activeBeacon>=0?teal:muted);
    r.text({243,570},timeText(game.time),13,white,true);
    panel({649,547,532,36},ammo);
    for (int i=0;i<static_cast<int>(weaponCount);++i) {
        const float x=662.f+i*74.f;
        if (i==game.gun) r.slab({x-5,553,72,24},5,hex(0x29404b),hex(0x1c2d37));
        r.text({x,569},std::to_string(i+1)+" "+weapons[i].shortName,11.5f,
               !game.guns[i].available?hex(0x647382):i==game.gun?ammo:white,true);
    }
    if (game.coop) {
        const auto& partner=game.teammate();
        const Color tag=hex(partner.character->color);
        const Rect tile{19,116,260,30};
        panel(tile,tag);
        label({31,136},"PARTNER  /  "+std::to_string(partner.player.hp)+" SUIT",tag);
        const float distance=partner.player.box.center().x-p.box.center().x;
        label({185,136},std::abs(distance)<50?"NEARBY":(distance<0?"< ":" > ")+std::to_string(int(std::abs(distance)))+" PX",pale);
    }
    // Combo counter and style call-outs.
    if (game.combo>=2 && game.comboTimer>0) {
        const float pop=1+.5f*std::exp(-(3.2f-game.comboTimer)*12);
        const Color tint=game.combo>=5?hex(0xff5f7e):game.combo>=3?hex(0xffd35c):hex(0x7ff2ff);
        const std::string count="x"+std::to_string(game.combo);
        r.outlinedText({1105-r.measure(count,54*pop,true)*.5f,170},count,54*pop,tint);
        r.slab({1008,180,170,6},5,ink.alpha(.8f),ink.alpha(.8f));
        r.slab({1008,180,170*clamp(game.comboTimer/3.2f),6},5,tint,tint.energy(.7f));
    }
    if (game.style.life>0) {
        const auto& s=game.style;
        const float age=s.maxLife-s.life;
        const float pop=1+.65f*std::exp(-age*13);
        const float size=30*pop;
        const float alpha=clamp(s.life*3.5f);
        const float x=width*.5f-r.measure(s.text,size,true)*.5f;
        r.outlinedText({x,150-(1-alpha)*10},s.text,size,s.color.alpha(alpha));
    }
    if (game.entry>0) {
        const float a=clamp(game.entry*1.17f);
        label({55,157},game.stage->location,accent.alpha(a));
        r.outlinedText({51,213},game.stage->name,52,white.alpha(a));
        if (game.index==0) {
            r.text({55,243},"SHIFT SPRINT   /   C CROUCH BEHIND BARRICADES   /   JUMP TWICE TO FLIP   /   Q FOR BULLET TIME",12,pale.alpha(a),true);
        }
    }
    if (view==View::play && !coopPaused && Rect{0,0,width,height}.contains(mouse) && !input.aimStickActive()) {
        const Color mark=mix(white,ammo,.45f).alpha(.95f);
        const float spin=clock*(slow?2.4f:.6f);
        const float radius=slow?13.f:9.f;
        for (int i=0;i<4;++i) {
            const float a=spin+i*pi*.5f;
            r.line(mouse+ray(a)*(radius-2),mouse+ray(a)*(radius+5),2.4f,ink.alpha(.9f));
            r.line(mouse+ray(a)*(radius-1),mouse+ray(a)*(radius+4),1.2f,mark);
        }
        r.ellipse(mouse,{2.4f,2.4f},ink); r.ellipse(mouse,{1.4f,1.4f},mark);
        // Hit marker: four short ticks that open outward on a hit and turn gold and larger on a kill.
        const float kill=clamp(game.killMarker/.45f), hit=clamp(game.hitMarker/.2f);
        const float strength=std::max(kill,hit);
        if (strength>0) {
            const Color tint=kill>0?hex(0xffd35c):hex(0xffffff);
            const float open=(kill>0?15.f:12.f)+(1-strength)*(kill>0?9.f:5.f);
            for (int i=0;i<4;++i) {
                const Vec d=ray(pi*.25f+i*pi*.5f);
                r.line(mouse+d*open,mouse+d*(open+(kill>0?9.f:6.f)),3.2f,ink.alpha(.85f*strength));
                r.line(mouse+d*open,mouse+d*(open+(kill>0?9.f:6.f)),1.8f,tint.alpha(strength));
            }
        }
    }
}
} // namespace vl
