#pragma once
#include "game/game.hpp"
#include "net/session.hpp"

namespace vl::net {
struct Sound {
    std::string name;
    float gain=1,pan=0,rate=1;
};
Bytes encodeControls(const Controls& controls,uint32_t mission);
bool decodeControls(std::span<const uint8_t> bytes,Controls& controls,uint32_t& mission);
Bytes encodeMission(int sector,int difficulty,uint32_t mission);
bool decodeMission(std::span<const uint8_t> bytes,int& sector,int& difficulty,uint32_t& mission);
Bytes encodeWorld(const Game& game,const std::vector<Sound>& sounds);
bool decodeWorld(std::span<const uint8_t> bytes,Game& game,std::vector<Sound>& sounds);
}
