#pragma once
#include "content/types.hpp"
#include <cmath>
#include <optional>

namespace vl {
inline float stairY(const Stair& s,float x) {
    return s.from.y+(s.to.y-s.from.y)*clamp((x-s.from.x)/(s.to.x-s.from.x));
}
inline bool validStair(const Stair& s) {
    const float dx=s.to.x-s.from.x;
    return std::isfinite(s.from.x)&&std::isfinite(s.from.y)&&
        std::isfinite(s.to.x)&&std::isfinite(s.to.y)&&
        dx>=48.f && dx<=700.f && std::abs(s.to.y-s.from.y)<=dx*.95f;
}
// Extraction gate needs a flat approach for a full-size operative: no
// staircase may intersect its 100px arrival zone (at gate elevation).
inline bool exitStairConflict(const Stair& stair,Vec exit) {
    const float low=std::min(stair.from.x,stair.to.x);
    const float high=std::max(stair.from.x,stair.to.x);
    const float nearestY=std::min(std::abs(stair.from.y-exit.y),
                                  std::abs(stair.to.y-exit.y));
    return high>=exit.x-105.f && low<=exit.x+35.f && nearestY<160.f;
}

// Finds the closest step surface under a foot, without teleporting characters
// up from a lower floor or catching them high above a staircase.
inline std::optional<float> stairFooting(const Stage& stage,float x,float feet,float tolerance=18.f) {
    std::optional<float> found;
    float best=tolerance;
    for (const Stair& s:stage.stairs) {
        if (!validStair(s) || x<s.from.x || x>s.to.x) continue;
        const float y=stairY(s,x);
        if (const float error=std::abs(y-feet);error<=best) { found=y;best=error; }
    }
    return found;
}
} // namespace vl
