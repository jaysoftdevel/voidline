#pragma once
#include "core/math.hpp"

namespace vl::story {
inline constexpr size_t missionPreloads=9;
struct Shot {
    const char* art;
    const char* location;
    const char* title;
    const char* speaker;
    const char* line;
    uint32_t tint;
    float at;
    Vec start,end;
};
inline constexpr std::array opening{
    Shot{"orbital","VEGA-9  /  YEAR 2187","A city below the ice","COLONY ARCHIVE",
         "Eleven thousand people came here to mine ice. In the dark season, the heat from the mines kept the whole colony alive.",
         0x85b6e3,0,{.25f,.39f},{.68f,.54f}},
    Shot{"tideworks","TIDEWORKS  /  SHIFT 63","The night the pumps stopped","SHIFT LOG  /  02:13",
         "When the coolant pumps failed, Helix sealed the transit shafts. It diverted the last power to the ore lifts and cut every civilian transmitter.",
         0x6eced6,5.2f,{.72f,.46f},{.39f,.60f}},
    Shot{"overgrowth","RESEARCH DOME  /  LAST CIVILIAN SIGNAL","A voice from below","DISTRESS RECORDING",
         "We got the children down to the old mine. The machines are guarding the stairs. If you hear this, get the relay back on.",
         0x9bd6a6,10.5f,{.34f,.54f},{.66f,.42f}},
    Shot{"cinematic/sky","RESCUE CUTTER  /  FINAL APPROACH","One way through","RESCUE COMMS",
         "Follow the high route until you see the mining lift. The sentries below are still active. Keep a channel open.",
         0xf1a170,15.9f,{.73f,.47f},{.42f,.54f}},
    Shot{"terminus","ORBITAL RELAY  /  BLACKOUT","Bring the relay back","RESCUE COMMS",
         "The relay will give the evacuation ships a way in. The only route is from below, past Mule-7.",
         0xadc2fb,21.1f,{.35f,.41f},{.68f,.51f}},
    Shot{"descent","LOWER MINE  /  INSERTION","Chapter 1. The Descent","TEAM COMMS",
         "Landing lights ahead. Check your suit and stay on comms.",
         0xf4af7b,26.1f,{.42f,.58f},{.62f,.49f}}
};
}
