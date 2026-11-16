#pragma once
// Side-on human running cycle. One normalized cycle contains two alternating
// steps; phases 0 and 0.5 are opposite-foot strikes. A running step has a
// short planted interval and a longer recovery interval with a flight phase.
// This is pure animation math: collision, speed and authored model files remain
// independent of how the renderer draws the feet.
#include <algorithm>
#include <cmath>

namespace vl::running {
inline constexpr float pi=3.14159265358979323846f;
inline float unit(float v) {
    const float phase=std::fmod(v,1.f);
    return phase<0.f?phase+1.f:phase;
}
inline float ease(float x) {
    x=std::clamp(x,0.f,1.f);
    return x*x*(3.f-2.f*x);
}
struct Step {
    bool planted=false;
    float progress=0.f; // 0 at toe-off; 1 at the next heel strike
    float clearance=0.f; // normalized vertical lift for the recovering foot
    float pitch=0.f; // positive: toes up at landing; negative: toe-off
};
inline Step sample(float phase,float stance) {
    phase=unit(phase);
    stance=std::clamp(stance,.27f,.75f);
    if(phase<stance) {
        const float t=phase/stance;
        // The landing foot rolls from heel through flat contact to the toes.
        const float heel=.28f*(1.f-ease(t/.27f));
        const float toe=-.43f*ease((t-.66f)/.34f);
        return {true,t,0.f,heel+toe};
    }
    const float t=(phase-stance)/(1.f-stance);
    // The heel folds rapidly behind the body, then the knee drives through;
    // the foot reaches forward and extends before heel strike. Both ends
    // land at zero height and zero horizontal acceleration.
    const float arc=std::max(0.f,std::sin(pi*t));
    const float lift=std::pow(arc,1.12f)*(1.10f-.20f*t);
    const float pitch=-.43f*(1.f-ease(t/.36f))+.28f*ease((t-.69f)/.31f);
    return {false,ease(t),lift,pitch};
}
// In a side view the pelvis follows the stepping leg while the shoulder
// girdle counter-rotates. This is a signed, two-step oscillation: reversing
// the legs half a cycle later reverses the rotation, without changing the
// overall rise/fall cadence of the torso.
inline float girdleTurn(float phase) {
    return std::sin(unit(phase)*2.f*pi);
}
// The visible feet, not a separate sine-wave clock, determine torso twist.
// Positive lead means the near (left/profile-facing) foot is forward along
// the direction of travel; negative lead means the far foot is forward.
// Taking the difference makes the motion symmetric when the legs exchange.
inline float footLead(float nearHeelX,float farHeelX,float travelAxis) {
    const float direction=travelAxis<0.f?-1.f:1.f;
    return std::clamp((nearHeelX-farHeelX)*direction/28.f,-1.f,1.f);
}
struct ShoulderOffsets { float near=0.f,far=0.f; };
// Shoulder motion is equal and opposite around the chest center. When a foot
// leads, its same-side shoulder goes behind the torso; its opposite shoulder
// moves forward. Values here are in travel-local coordinates.
inline ShoulderOffsets counterShoulders(float lead,float travelStrength) {
    const float swing=std::clamp(lead,-1.f,1.f)*travelStrength;
    return {-swing,swing};
}
inline float hipOffset(float phase) {
    // Two vertical impulses per complete left/right running cycle: the body
    // compresses as weight loads, then rises during push-off and flight.
    const float t=unit(phase*2.f);
    if(t<.19f) return 1.2f+1.0f*ease(t/.19f);
    if(t<.66f) return 2.2f-4.8f*ease((t-.19f)/.47f);
    return -2.6f+3.8f*ease((t-.66f)/.34f);
}
} // namespace vl::running
