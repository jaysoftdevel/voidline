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
// Original seven-scene prologue. Every scene is animated in the renderer, while
// post-sector story pages intentionally remain static (per the player's preference).
inline constexpr std::array opening{
    Shot{"orbital","VEGA-9  /  EUROPA ORBIT  /  2187","The last warm city","COLONY RECORD  /  ARCHIVE 01",
         "Under Europa's ice, eleven thousand colonists built a city around the deep mines. The reactors and the pumps kept the shelters alive.",
         0x87bded,0,{.29f,.38f},{.65f,.52f}},
    Shot{"tideworks","TIDEWORKS  /  DARK SHIFT 63","When the pumps went silent","CIVILIAN EMERGENCY CHANNEL",
         "At 02:13 the coolant pumps stopped. Floodwater entered the workers' corridors. Automated doors sealed, trapping families below ground.",
         0x6ad5e0,4.5f,{.76f,.48f},{.34f,.59f}},
    Shot{"cinematic/relay","HELIX CONTROL  /  TRANSIT SHUTDOWN","The order came from inside","RECOVERED MACHINE LOG",
         "Helix diverted the colony's remaining power to the ore lifts. Rescue beacons were suppressed. Security machines began hunting survivors.",
         0xd78d77,9.0f,{.29f,.49f},{.69f,.46f}},
    Shot{"overgrowth","RESEARCH DOME  /  LAST TRANSMISSION","A voice beneath the ice","DR. NERI  /  DISTRESS RECORDING",
         "We're in the old mine with the children. We have six hours of heat left. Find the uplink. Please tell the ships we're still here.",
         0xa4e0a9,13.5f,{.30f,.55f},{.72f,.38f}},
    Shot{"cinematic/sky","RESCUE CUTTER ASTER  /  FINAL APPROACH","One ship answered","ASTER FLIGHT DECK",
         "A rescue cutter broke through the radio silence. Its crew could land one operative team, but the sealed orbital relay kept evacuation ships away.",
         0xf5aa78,18.0f,{.79f,.41f},{.42f,.56f}},
    Shot{"terminus","TERMINUS  /  ORBITAL UPLINK","Twenty sectors to daylight","MISSION COMMAND",
         "Reach the old relay from the mine floor, unlock the lifts and open the sky. Every sector between you and Terminus is still defended.",
         0xb5baff,22.5f,{.38f,.42f},{.68f,.52f}},
    Shot{"descent","LOWER MINE  /  INSERTION POINT","Chapter 1. The Descent","TEAM COMMS",
         "The cutter is leaving. The air is thin, the machines are awake, and someone below is waiting. Move out. Bring them home.",
         0xffc189,27.0f,{.44f,.57f},{.63f,.48f}}
};
}
