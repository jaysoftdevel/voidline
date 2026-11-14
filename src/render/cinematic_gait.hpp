#pragma once
#include "core/math.hpp"

namespace vl {
inline constexpr float cinematicStride=36.f;

// Quintic interpolation: zero velocity AND zero acceleration at both ends.
inline float cinematicEase(float t) {
    t=clamp(t);
    return t*t*t*(10.f+t*(-15.f+6.f*t));
}

// Distance for an accelerate / cruise / brake motion. The quintic velocity
// ramps integrate analytically, so the foot cycle cannot drift from the body.
inline float cinematicCruise(float time,float start,float stop,float ramp) {
    const float duration=std::max(stop-start,.001f);
    const float blend=std::clamp(ramp,.001f,duration*.5f);
    const float t=std::clamp(time-start,0.f,duration);
    const auto integral=[](float u) {
        u=clamp(u);
        return u*u*u*u*(2.5f+u*(-3.f+u));
    };
    float advance;
    if (t<blend) advance=blend*integral(t/blend);
    else if (t>duration-blend) advance=duration-blend*integral((duration-t)/blend)-blend;
    else advance=t-blend*.5f;
    return clamp(advance/(duration-blend));
}

inline float cinematicWalk(float time,float start,float stop,float ramp) {
    if (stop<=start) return 0.f;
    const float blend=std::clamp(ramp,.001f,(stop-start)*.5f);
    return cinematicEase((time-start)/blend)*cinematicEase((stop-time)/blend);
}

struct CinematicFoot {
    float contact,lift,pitch;
};
inline CinematicFoot cinematicFoot(float cycle) {
    const float phase=cycle-std::floor(cycle);
    constexpr float stance=.60f;
    const float swing=clamp((phase-stance)/(1-stance));
    const float arc=std::sin(pi*swing);
    // Stance displacement cancels the body's forward travel.
    return {cinematicStride*(stance*.5f-phase+smooth(swing)),arc*arc*6.f,arc*std::cos(pi*swing)*.47f};
}
}
