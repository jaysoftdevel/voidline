#pragma once
#include "game/entities.hpp"
#include <algorithm>
#include <cmath>

namespace vl {
// Restrict a walking enemy to continuous solid footing at its own floor height.
// Flying enemies and fixed turrets never call this. Use both the middle of the
// footprint and its leading foot so even wide enemies cannot stride into a pit.
inline bool enemyHasFooting(const Stage& stage,const Enemy& enemy,float x,int direction) {
    const auto supported=[&](float at) {
        const auto on=[&](const Rect& surface) {
            return std::abs(surface.y-enemy.base)<2.f &&
                at>=surface.x+1.f && at<=surface.right()-1.f;
        };
        for (const auto& surface:stage.ground) if (on(surface)) return true;
        for (const auto& surface:stage.ledges) if (on(surface)) return true;
        return false;
    };
    const float middle=x+enemy.box.w*.5f;
    const float leading=middle+float(direction)*std::min(enemy.box.w*.32f,13.f);
    return supported(middle) && supported(leading);
}

inline float groundedEnemyX(const Stage& stage,const Enemy& enemy,float from,float wanted) {
    const int direction=wanted>from?1:wanted<from?-1:0;
    if (!direction || !enemyHasFooting(stage,enemy,from,direction)) return from;
    if (enemyHasFooting(stage,enemy,wanted,direction)) {
        // A high-speed step cannot leap an unsupported slot in one simulation tick.
        const float stride=wanted-from;
        const int steps=std::max(1,int(std::ceil(std::abs(stride)/6.f)));
        for (int i=1;i<steps;++i)
            if (!enemyHasFooting(stage,enemy,from+stride*float(i)/steps,direction))
                return groundedEnemyX(stage,enemy,from,from+stride*float(i-1)/steps);
        return wanted;
    }
    // Advance right to the safe edge without ever placing the lead foot in air.
    float safe=from,unsafe=wanted;
    for (int i=0;i<12;++i) {
        const float probe=(safe+unsafe)*.5f;
        if (enemyHasFooting(stage,enemy,probe,direction)) safe=probe;
        else unsafe=probe;
    }
    return safe;
}
} // namespace vl
