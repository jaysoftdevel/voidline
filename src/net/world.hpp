#pragma once
#include "game/game.hpp"
#include "net/session.hpp"
#include "net/timing.hpp"

namespace vl::net {
struct Sound {
    std::string name;
    float gain=1,pan=0,rate=1;
};
Bytes encodeControls(const Controls& controls,uint32_t mission);
bool decodeControls(std::span<const uint8_t> bytes,Controls& controls,uint32_t& mission);
Bytes encodeMission(int sector,int difficulty,uint32_t mission,CoopMode mode=CoopMode::campaign);
bool decodeMission(std::span<const uint8_t> bytes,int& sector,int& difficulty,uint32_t& mission,CoopMode* mode=nullptr);
struct Timing {
    uint32_t mission=0;
    Millis first=0,second=0;
};
Bytes encodeTiming(Timing timing);
bool decodeTiming(std::span<const uint8_t> bytes,Timing& timing);
struct WorldFrame {
    uint32_t mission=1,tick=0;
};
Bytes encodeWorld(const Game& game,const std::vector<Sound>& sounds,WorldFrame frame={});
bool decodeWorld(std::span<const uint8_t> bytes,Game& game,std::vector<Sound>& sounds,WorldFrame* frame=nullptr);
}
