#include "game/game.hpp"

namespace vl {
// Three hand-paced events per mission. Triggering by exploration distance keeps
// them deterministic on the authoritative host while the client observes them
// through the usual sound/particle/world snapshot channel.
void Game::scriptedSetpieces() {
    if (!stage || mode!=Mode::active) return;
    const float progress=std::max(player.box.x,coop?partner.player.box.x:0.f)/stage->worldWidth;
    constexpr float thresholds[]{.23f,.48f,.73f};
    constexpr const char* events[5][3]{
        {"SUPPORT GANTRY COLLAPSE","SUBSURFACE DETONATION","LIFT CONTROL RESTARTED"},
        {"FLOODGATE PRESSURE SPIKE","PUMP STATION SURGE","LOCKDOWN ALARM"},
        {"SPORE CLOUD RELEASE","ROOT SYSTEM AWAKENING","CONTAINMENT BREACH"},
        {"SLAG VALVE OPEN","COOLANT PUMP FAILURE","FURNACE PRESSURE WAVE"},
        {"SECURITY UPLINK ACTIVE","GRID POWER SURGE","ORBITAL DEFENSE ALERT"}
    };
    for (int i=0;i<3;++i) {
        const uint32_t bit=1u<<i;
        if (progress<thresholds[i] || (setpieceMask&bit)) continue;
        setpieceMask|=bit;
        const Vec spot{player.box.center().x+220,player.box.center().y-65};
        const Color tint=hex(stage->accent);
        message(spot-Vec{0,74},events[static_cast<int>(stage->biome)][i],tint,2.5f);
        ring(spot,tint,i==2?110.f:80.f,.55f);
        burst(spot,hex(0xffc27d),i==2?34:19,150+50*i);
        burst(spot,hex(0x758593),18,90,ParticleKind::puff);
        spawnAmbient(static_cast<int>(i==1?AmbientKind::lightning:AmbientKind::blast),i%2);
        skyFlash=std::max(skyFlash,.18f+.13f*i);
        shake=std::max(shake,3.f+i*1.7f);
        audio.event("launch",.33f,0,.7f+.13f*i);
    }
}
}
