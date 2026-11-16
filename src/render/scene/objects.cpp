#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::objects(const Game& g) {
    const Color cold=hex(g.stage->cold),tone=metal(g.stage->biome);
    if (g.puzzleMode) {
        // A clear shared insertion bay and readable fork. These markers are
        // environmental art; collision is handled by the actual walkways.
        const float floor=g.stage->ground.front().y;
        const Vec staging{124.f+offset.x,floor+offset.y};
        if (visible(124,220)) {
            rect({staging.x-82,staging.y-6,178,6},hex(0x173443));
            line({staging.x-80,staging.y-7},{staging.x+96,staging.y-7},2.f,hex(0x7ceaca).energy(1.6f));
            text({staging.x-76,staging.y-85},"SHARED INSERTION  /  TEAM DEPLOYMENT",10,hex(0xe3f9fc),true);
            text({staging.x-76,staging.y-66},"MOVE RIGHT TO SPLIT ROUTES",9,hex(0xaacbd9),true);
        }
        const Vec fork{286.f+offset.x,floor+offset.y};
        if (visible(286,200)) {
            line(fork+Vec{-15.f,-20.f},fork+Vec{-15.f,-142.f},2.f,hex(0x7edfff).energy(1.6f));
            line(fork+Vec{-15.f,-142.f},fork+Vec{-21.f,-131.f},2.f,hex(0x7edfff));
            line(fork+Vec{-15.f,-142.f},fork+Vec{-9.f,-131.f},2.f,hex(0x7edfff));
            line(fork+Vec{24.f,-21.f},fork+Vec{100.f,-21.f},2.f,hex(0xffbf75).energy(1.4f));
            line(fork+Vec{100.f,-21.f},fork+Vec{90.f,-27.f},2.f,hex(0xffbf75));
            line(fork+Vec{100.f,-21.f},fork+Vec{90.f,-15.f},2.f,hex(0xffbf75));
            text(fork+Vec{-128.f,-155.f},"PLAYER 1  /  UPPER SWITCHES",9,hex(0x99ddff),true);
            text(fork+Vec{30.f,-34.f},"PLAYER 2  /  LOWER ROUTE",9,hex(0xffce8e),true);
        }
    }
    if (g.puzzleMode) for (size_t i=0;i<g.stage->relays.size();++i) {
        const auto& relay=g.stage->relays[i];
        const auto& state=g.relayStates[i];
        const bool active=g.relayOpen(i);
        const Color ready=hex(0x7effcf), warning=hex(0xff8e67), steel=hex(0x506c7c);
        const Color tint=active?ready:warning;
        const Vec pad=relay.pad+offset;
        if (visible(relay.pad.x,120)) {
            if (relay.control==RelayControl::pressurePlate) {
                // Floor switch: a sprung plate visibly sinks under the operator.
                const float depression=state.charge*4.f;
                rect({pad.x-48,pad.y-7,96,9},hex(0x162835));
                rect({pad.x-43,pad.y-8+depression,86,6},steel.energy(.75f));
                rect({pad.x-35,pad.y-10+depression,70*state.charge,3},ready.energy(1.7f));
                for (int k=0;k<4;++k) rect({pad.x-38+k*23,pad.y-6+depression,13,2},tint.alpha(.8f));
                outline({pad.x-48,pad.y-7,96,10},hex(0x67808f));
                text({pad.x-44,pad.y-30},"STAND ON PLATE",10,hex(0xd8f9fa),true);
            } else {
                // A mounted lever with an actual rotating handle and charge panel.
                const Vec mount{pad.x,pad.y-28};
                rect({mount.x-16,mount.y-5,32,30},hex(0x14212c));
                outline({mount.x-16,mount.y-5,32,30},steel);
                ellipse(mount,{7,7},hex(0x283d4a));
                const float angle=-1.15f+state.charge*1.95f;
                const Vec grip=mount+Vec{std::sin(angle)*23.f,-std::cos(angle)*23.f};
                line(mount,grip,5,steel.energy(1.1f));
                ellipse(grip,{6,6},tint.energy(1.8f));
                rect({mount.x-10,mount.y+12,20*state.charge,4},ready.energy(1.7f));
                text({pad.x-43,pad.y-94},"HOLD E  /  OPERATE LEVER",9,hex(0xd8f9fa),true);
                if (state.lease>0 && !state.latched)
                    text({pad.x-34,pad.y-78},"OPEN "+std::to_string(int(std::ceil(state.lease)))+" SEC",10,ready,true);
            }
            text({pad.x-43,pad.y+22},relay.operatorPlayer==0?"P1 CONTROLS / P2 CROSSES":"P2 CONTROLS / P1 CROSSES",9,hex(0xc9eff4),true);
            halo(pad-Vec{0,12},15+state.charge*20,ready.alpha(.11f+state.charge*.28f));
        }
        if (visible(relay.barrier.x,100)) {
            const Rect b{relay.barrier.x+offset.x,relay.barrier.y+offset.y,
                         relay.barrier.w,relay.barrier.h};
            // Mechanized bulkhead: guide rails, upper motor, two sliding
            // shutter leaves, moving seams, and a status strip.
            // Side rails only: the opening itself must stay transparent once
            // the shutter retracts, so the characters and world remain visible.
            rect({b.x-11,b.y-12,10,b.h+14},hex(0x0b1823));
            rect({b.right()+1,b.y-12,10,b.h+14},hex(0x0b1823));
            rect({b.x-8,b.y-9,6,b.h+9},steel.energy(.7f));
            rect({b.right()+3,b.y-9,6,b.h+9},steel.energy(.7f));
            const float shutter=1.f-state.openness;
            if (shutter>.003f) {
                const float visibleHeight=b.h*shutter;
                const Rect leaf{b.x+2,b.y,b.w-4,visibleHeight};
                gradient(leaf,hex(0x3d5360).alpha(.97f),hex(0x111e29).alpha(.99f));
                for (float y=leaf.y+11;y<leaf.bottom()-5;y+=19.f) {
                    line({leaf.x+3,y},{leaf.right()-3,y},2.f,hex(0x708998).alpha(.54f));
                    rect({leaf.x+4,y-3,6,5},tint.energy(1.15f));
                }
                line({leaf.x+1,leaf.bottom()-3},{leaf.right()-1,leaf.bottom()-3},3.f,tint.energy(1.4f));
            }
            rect({b.x-14,b.y-18,b.w+28,16},hex(0x243844));
            outline({b.x-14,b.y-18,b.w+28,16},steel);
            for (int k=0;k<3;++k) ellipse({b.x+11+k*12.f,b.y-10},{2.8f,2.8f},tint.energy(active?2.f:1.3f));
            halo({b.center().x,b.y-10},24,tint.alpha(.26f));
            text({b.x-18,b.y-27},state.latched?"SECURED":active?"DOOR OPEN":"SEALED DOOR",10,tint,true);
        }
    }
    for (const auto& vent:g.stage->vents) {
        if (!visible(vent.x)) continue;
        const Vec p{vent.x+offset.x,vent.y+offset.y};
        const int state=g.ventState(vent.phase);
        const Color hazard=hex(g.stage->biome==Biome::biodome?0xbfff92:g.stage->biome==Biome::orbital?0xbd9aff:0xff9b62);
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
    for (int i=0;i<static_cast<int>(g.stage->beacons.size());++i) {
        const auto& at=g.stage->beacons[i];
        if (!visible(at.x)) continue;
        const Vec beacon=at+offset;
        const Color status=i==g.activeBeacon?hex(0x92ffbe):cold;
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
        if (!g.bossDead) {
            // Solid powered shutter until the boss falls. The player may
            // still reach the outside of the gate and inspect its status.
            for (int i=0;i<8;++i) {
                rect({exit.x-29,exit.y-82+i*9.f,58,5},tone.energy(.66f));
                rect({exit.x-26,exit.y-81+i*9.f,52,1},status.alpha(.36f));
            }
        } else {
            // Clear aperture and retracted shutters after the boss defeat.
            rect({exit.x-29,exit.y-84,4,84},status.alpha(.36f));
            rect({exit.x+25,exit.y-84,4,84},status.alpha(.36f));
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
    for (const auto& crate:g.crates) {
        if (!visible(crate.box.x,crate.box.w)) continue;
        const auto& spec=crateSpec(crate.kind);
        const Color tint=hex(spec.color);
        const Vec p=crate.box.center()+offset;
        const Rect b{crate.box.x+offset.x,crate.box.y+offset.y,crate.box.w,crate.box.h};
        if (crate.spent) {
            line({b.x+2,b.bottom()-3},{b.right()-2,b.bottom()-3},4,hex(0x242428));
            continue;
        }
        gradient(b,mix(tone,tint,.32f),hex(0x1d222c));
        outline(b,hex(0x111820),2);
        rect({b.x+3,b.y+3,b.w-6,4},tint.energy(1.45f));
        rect({b.x+4,b.y+8,3,b.h-13},tint.alpha(.68f));
        rect({b.right()-7,b.y+8,3,b.h-13},tint.alpha(.68f));
        const float pip=(b.w-12)/spec.hits;
        for (int i=0;i<spec.hits;++i)
            rect({b.x+6+i*pip,b.bottom()-7,pip-1,2},tint.alpha(i<crate.hits?.95f:.20f));
        const int marks=static_cast<int>(crate.kind)+1;
        for (int i=0;i<marks;++i) {
            const float x=p.x+(i-(marks-1)*.5f)*7;
            line({x-2,p.y+3},{x+2,p.y-4},2,tint.energy(1.45f));
        }
        halo(p,27,tint.alpha(crate.hits<spec.hits?.27f+.14f*std::sin(g.time*13):.22f));
    }
    auto projectile=[&](const Shot& s,bool friendly) {
        const Vec at=shotPosition(s,friendly);
        if (!visible(at.x)) return;
        const Vec p=at+offset, dir=normal(s.v);
        const float tail=s.longShot?34.f:s.wave?23.f:friendly?16.f:11.f;
        halo(p,s.longShot?22:14,s.color.energy(1.40f).alpha(.48f));
        line(p-dir*tail,p+dir*3,s.wave?6.3f:3.9f,s.color.energy(s.longShot?2.6f:1.91f).alpha(.78f));
        line(p-dir*tail*.58f,p+dir*3,s.wave?2.6f:1.5f,hex(0xfff5e4).energy(2.2f));
    };
    for (const auto& s:g.shots) projectile(s,true);
    for (const auto& s:g.hostile) projectile(s,false);
}
} // namespace vl
