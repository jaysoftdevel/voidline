#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::objects(const Game& g) {
    const Color cold=hex(g.stage->cold),tone=metal(g.index);
    for (const auto& vent:g.stage->vents) {
        if (!visible(vent.x)) continue;
        const Vec p{vent.x+offset.x,vent.y+offset.y};
        const int state=g.ventState(vent.phase);
        const Color hazard=hex(g.index==2?0xbfff92:g.index==4?0xbd9aff:0xff9b62);
        rect({p.x-21,p.y-4,42,4},tone.energy(.68f));
        for (int i=0;i<7;++i) rect({p.x-17+i*5,p.y-5,2,3},hazard.energy(state?1.8f:.51f));
        if (state) halo(p-Vec{0,14},42,hazard.alpha(state==2?.48f:.29f));
        if (state==2) for (int i=0;i<11;++i) {
            const float t=std::fmod(g.time*(.82f+hash(i)*.61f)+i*.111f,1.f);
            const Vec start=p+Vec{std::sin(i*2.3f+t*5)*(7+t*14),-t*70};
            const Color c=hazard.energy(1.5f).alpha((1-t)*.71f);
            line(start,start+Vec{std::sin(i*4.f)*8,-8-t*14},2.2f,c);
        }
    }
    const auto beacon=g.stage->beacon+offset;
    if (visible(g.stage->beacon.x)) {
        const Color status=g.checkpoint?hex(0x92ffbe):cold;
        rect({beacon.x-22,beacon.y-4,44,4},tone.energy(.80f));
        rect({beacon.x-11,beacon.y-46,22,44},hex(0x16232b));
        outline({beacon.x-11,beacon.y-46,22,44},tone.energy(.94f));
        rect({beacon.x-2,beacon.y-38,4,28},status.energy(1.67f));
        halo(beacon-Vec{0,41},32,status.alpha(.51f));
        ellipse(beacon-Vec{0,47},{6,3},status.energy(2.f));
    }
    const auto exit=g.stage->exit+offset;
    if (visible(g.stage->exit.x)) {
        const Color status=g.bossDead?hex(0x85ffd4):hex(0xff694f);
        gradient({exit.x-45,exit.y-109,90,109},tone.energy(.8f),tone.energy(.42f));
        rect({exit.x-32,exit.y-90,64,90},hex(0x101b26));
        rect({exit.x-29,exit.y-84,58,84},hex(0x06121b));
        for (int i=0;i<8;++i) {
            rect({exit.x-29,exit.y-82+i*9.f,58,2},tone.energy(.38f));
            if (g.bossDead) rect({exit.x-26,exit.y-81+i*9.f,52,1},status.alpha(.33f));
        }
        rect({exit.x-32,exit.y-89,3,85},status.energy(1.8f));
        rect({exit.x+29,exit.y-89,3,85},status.energy(1.8f));
        rect({exit.x-44,exit.y-108,88,13},hex(0x15202a));
        halo(exit-Vec{0,84},58,status.alpha(.43f));
        text({exit.x-32,exit.y-98},g.bossDead?"EXTRACTION":"LOCKED",8,status.energy(1.55f),true);
    }
    for (size_t i=0;i<g.supplies.size();++i) {
        const auto& s=g.supplies[i];
        if (s.used || !visible(s.p.x)) continue;
        const Color c=supplyColor(s.kind);
        const Vec p=s.p+offset+Vec{0,std::sin(g.time*4+float(i)) * 2};
        halo(p,26,c.alpha(.44f));
        gradient({p.x-11,p.y-9,22,17},hex(0x465761),hex(0x15222c));
        outline({p.x-11,p.y-9,22,17},c.energy(.77f));
        rect({p.x-8,p.y-6,2,11},c.energy(1.49f));
        rect({p.x+6,p.y-6,2,11},c.energy(1.49f));
        if (s.kind==SupplyKind::health) {
            rect({p.x-1,p.y-5,2,9},c.energy(1.8f));
            rect({p.x-4,p.y-2,8,2},c.energy(1.8f));
        } else if (s.kind==SupplyKind::ammo) {
            for (int j=-3;j<=3;j+=3) rect({p.x+j,p.y-4,1.5f,7},c.energy(1.68f));
        } else {
            ellipse(p,{4,4},c.energy(1.64f));
        }
    }
    auto projectile=[&](const Shot& s,bool friendly) {
        if (!visible(s.p.x)) return;
        const Vec p=s.p+offset, dir=normal(s.v);
        const float tail=s.longShot?34.f:s.wave?23.f:friendly?16.f:11.f;
        halo(p,s.longShot?22:14,s.color.energy(1.40f).alpha(.48f));
        line(p-dir*tail,p+dir*3,s.wave?6.3f:3.9f,s.color.energy(s.longShot?2.6f:1.91f).alpha(.78f));
        line(p-dir*tail*.58f,p+dir*3,s.wave?2.6f:1.5f,hex(0xfff5e4).energy(2.2f));
    };
    for (const auto& s:g.shots) projectile(s,true);
    for (const auto& s:g.hostile) projectile(s,false);
}
} // namespace vl
