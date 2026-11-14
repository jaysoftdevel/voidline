#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
int Game::ventState(float phase) const {
    const float cycle=std::fmod(time+phase,3.65f);
    return cycle>2.76f?2:cycle>2.22f?1:0;
}

void Game::triggers() {
    auto& p=player;
    for (auto& item:supplies) {
        if (item.used || std::abs(item.p.x-p.box.center().x)>29 || std::abs(item.p.y-(p.box.bottom()-18.f))>31) continue;
        std::string text;
        if (item.kind==SupplyKind::health) {
            if (p.hp>=maxHealth) continue;
            p.hp=std::min(maxHealth,p.hp+int(pickupHeal)); text="SUIT REPAIRED";
        } else if (item.kind==SupplyKind::salvage) {
            const int points=std::lround(22*difficulty->score*(1+.25f*campaign.perks[3]));
            score+=points; text="+"+std::to_string(points)+" SCORE";
        } else {
            bool resupplied=false;
            for (auto& g:guns) {
                if (!g.available || g.reserve<0 || g.reserve>=g.maxReserve) continue;
                g.reserve=std::min(g.maxReserve,g.reserve+int(std::ceil(g.maxReserve*pickupAmmo)));
                resupplied=true;
            }
            if (!resupplied) continue;
            text="AMMO RESUPPLIED";
        }
        const Color tint=hex(item.kind==SupplyKind::health?0x9dedb4:0xffd190);
        item.used=true; message(item.p-Vec{0,16},text,tint); burst(item.p,tint,10,105);
        audio.event("pickup",1,0,.95f+random()*.10f);
    }
    for (const auto& v:stage->vents) {
        if (ventState(v.phase)==2 && p.box.overlaps({v.x-19,v.y-75,38,75})) hurt(12+std::min(index,4)*2+std::max(0,index-4));
    }
}

void Game::checkpoints() {
    auto near=[](const Player& p,Vec beacon) {
        return std::abs(p.box.center().x-beacon.x)<44 && std::abs(p.box.bottom()-beacon.y)<155;
    };
    for (int i=activeBeacon+1;i<static_cast<int>(stage->beacons.size());++i) {
        const Vec beacon=stage->beacons[i];
        // Co-op checkpoints cannot bypass a still-locked earlier gate.
        if (puzzleMode) {
            bool ready=true;
            for (size_t j=0;j<stage->relays.size();++j)
                if (stage->relays[j].barrier.x<beacon.x && !relayStates[j].latched) ready=false;
            if (!ready) continue;
        }
        if (!near(player,beacon) && (!coop || !near(partner.player,beacon))) continue;
        activeBeacon=i; player.hp=maxHealth;
        if (coop) partner.player.hp=partner.maxHealth;
        backup={time,score,kills,gun,bossActive,bossDead,guns,enemies,supplies,crates,partner,relayStates};
        message(beacon-Vec{0,70},"CHECKPOINT ONLINE",hex(0xb9ffcf),1.75f);
        audio.event("checkpoint");
        break;
    }
}
} // namespace vl
