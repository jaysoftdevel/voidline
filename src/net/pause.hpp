#pragma once
#include "net/session.hpp"
#include <cstdint>
#include <span>

namespace vl::net {
enum class PauseOwner : uint8_t { host=0, guest=1 };
struct PauseState { bool active=false; PauseOwner owner=PauseOwner::host; };
// Only the player who paused can resume. Host is the single authority.
inline bool changePause(PauseState& state,PauseOwner caller,bool wantPaused) {
    if (wantPaused) {
        if (state.active) return false;
        state={true,caller};
    } else {
        if (!state.active || state.owner!=caller) return false;
        state={false,caller};
    }
    return true;
}
inline Bytes encodePauseRequest(uint32_t mission,bool pause) {
    return {uint8_t(mission),uint8_t(mission>>8),uint8_t(mission>>16),uint8_t(mission>>24),uint8_t(pause)};
}
inline bool decodePauseRequest(std::span<const uint8_t> bytes,uint32_t& mission,bool& pause) {
    if (bytes.size()!=5 || bytes[4]>1) return false;
    mission=uint32_t(bytes[0])|(uint32_t(bytes[1])<<8)|(uint32_t(bytes[2])<<16)|(uint32_t(bytes[3])<<24);
    pause=bytes[4]!=0;
    return mission!=0;
}
inline Bytes encodePauseState(uint32_t mission,PauseState state) {
    auto packet=encodePauseRequest(mission,state.active);
    packet.push_back(static_cast<uint8_t>(state.owner));
    return packet;
}
inline bool decodePauseState(std::span<const uint8_t> bytes,uint32_t& mission,PauseState& state) {
    bool paused=false;
    if (bytes.size()!=6 || bytes[5]>1 || !decodePauseRequest(bytes.first(5),mission,paused)) return false;
    state={paused,static_cast<PauseOwner>(bytes[5])};
    return true;
}
}
