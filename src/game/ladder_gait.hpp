#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace vl::ladder_gait {
// Ladder rungs are rendered every 18 world units, starting 11 below the top.
// Sample against their WORLD positions, not elapsed animation time. A stationary
// climber therefore keeps every planted foot/hand fixed on its rung; climbing
// down plays precisely the same step sequence backwards.
struct Pose {
    std::array<float,2> feet{};       // World-space contact height for each boot.
    std::array<float,2> hands{};      // World-space rung height for each hand.
    std::array<float,2> swing{};      // 0..1, foot clearance during its transfer.
    int stepping=0;
};

inline Pose sample(float feetY,float top,float bottom) {
    constexpr float spacing=18.f;
    const float first=top+11.f;
    const float final=first+std::floor(std::max(0.f,bottom-4.f-first)/spacing)*spacing;
    const float progress=(bottom-feetY)/spacing;
    const float whole=std::floor(progress);
    const int step=static_cast<int>(whole);
    const float fraction=progress-whole;
    // Each foot advances by two rungs, while the other supports the body.
    // The first/last 18% of the step remain planted instead of sliding.
    const float t=std::clamp((fraction-.18f)/.64f,0.f,1.f);
    const float eased=t*t*(3.f-2.f*t);
    const int moving=((step%2)+2)%2;
    const auto rung=[&](float index) { return final-spacing*index; };
    Pose pose;
    pose.stepping=moving;
    pose.feet[1-moving]=rung(whole);
    pose.feet[moving]=rung(whole-1.f)*(1.f-eased)+rung(whole+1.f)*eased;
    pose.swing[moving]=std::sin(eased*3.14159265358979323846f);
    // Hands alternate diagonally with the legs and grip two rungs higher.
    // A hand remains fixed while its opposite foot is making a step.
    pose.hands[0]=pose.feet[1]-spacing*2.f;
    pose.hands[1]=pose.feet[0]-spacing*2.f;
    return pose;
}
} // namespace vl::ladder_gait
