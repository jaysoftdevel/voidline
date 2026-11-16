#pragma once
#include "core/math.hpp"

namespace vl::story {
inline constexpr size_t missionPreloads=9;
inline constexpr const char* openingArt="orbital";
inline constexpr size_t openingArtCount=1;
struct Shot {
    const char* location;
    const char* title;
    const char* speaker;
    const char* line;
    uint32_t tint;
    float at;
    Vec start,end;
};
// Exactly ONE screen: no per-chapter slides, transitions, or page changes.
inline constexpr std::array opening{
    Shot{"VEGA-9 / EUROPA / 2187","THE LAST TRANSMISSION","RESCUE CUTTER ASTER",
         "Helix sealed the colony. Survivors are trapped below the ice. Reach the orbital relay, disable the machines, and bring everyone home.",
         0x87e0c9,0.f,{.40f,.42f},{.59f,.51f}}
};
}
