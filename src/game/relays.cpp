#include "game/game.hpp"

namespace vl {
bool Game::relayOpen(size_t relayIndex) const {
    if (!stage || relayIndex>=relayStates.size() || relayIndex>=stage->relays.size()) return false;
    const auto& s=relayStates[relayIndex];
    return s.latched || s.charge>=.95f ||
        (stage->relays[relayIndex].control==RelayControl::lever && s.lease>0.f);
}

// All control evaluation is performed by the authoritative host. The world
// snapshot carries both the door travel and the temporary lever lease.
void Game::updateRelays(float dt) {
    if (!puzzleMode || !coop || !stage) return;
    for (size_t i=0;i<stage->relays.size();++i) {
        const auto& relay=stage->relays[i];
        auto& s=relayStates[i];
        const Player& owner=relay.operatorPlayer==0?player:partner.player;
        const Player& crossing=relay.operatorPlayer==0?partner.player:player;
        if (s.latched) { s.charge=1.f; s.openness=std::min(1.f,s.openness+dt*3.6f); continue; }
        const float distance=std::abs(owner.box.center().x-relay.pad.x);
        if (relay.control==RelayControl::pressurePlate) {
            const bool onPlate=distance<42.f && std::abs(owner.box.bottom()-relay.pad.y)<29.f;
            s.charge=onPlate?std::min(1.f,s.charge+dt*1.42f):std::max(0.f,s.charge-dt*3.4f);
        } else {
            const bool inReach=distance<54.f && std::abs(owner.box.bottom()-relay.pad.y)<75.f;
            const bool interacting=relay.operatorPlayer==0?held(Key::interact):
                partnerInput.keys[static_cast<size_t>(Key::interact)];
            const float prior=s.charge;
            s.charge=inReach && interacting?std::min(1.f,s.charge+dt*2.f):
                                              std::max(0.f,s.charge-dt*2.5f);
            if (prior<.95f && s.charge>=.95f) {
                s.lease=6.f;
                ring(relay.pad-Vec{0,23},hex(0x80ffe8),44,.36f);
                message(relay.pad-Vec{0,60},"LEVER ENGAGED  /  6 SEC",hex(0x9bffec),1.1f);
                audio.event("switch",.6f);
            }
            s.lease=std::max(0.f,s.lease-dt);
        }
        const bool opening=relayOpen(i);
        // A physical shutter travels into its header, rather than disappearing.
        // Keep a non-latched door open while its designated runner occupies it.
        const bool crossingDoor=crossing.box.overlaps(relay.barrier);
        const float target=(opening || (s.openness>.05f && crossingDoor))?1.f:0.f;
        s.openness=std::clamp(s.openness+(target>s.openness?1.f:-1.f)*dt*3.2f,0.f,1.f);
        if (opening && crossing.box.center().x>relay.barrier.right()+40.f) {
            s.latched=true; s.openness=1.f; s.lease=0.f;
            ring(relay.barrier.center(),hex(0x85ffe0),72,.6f);
            message(relay.barrier.center()-Vec{0,95},"DOOR SECURED",hex(0x85ffe0),1.8f);
            audio.event("checkpoint",.7f);
        }
    }
}
} // namespace vl
