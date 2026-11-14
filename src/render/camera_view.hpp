#pragma once
#include "core/math.hpp"

namespace vl {
class CameraTilt {
public:
    static Vec target(Vec pointer) {
        return {clamp(pointer.x*2/width-1,-1,1),clamp(pointer.y*2/height-1,-1,1)};
    }

    void advance(Vec goal,float dt) {
        axis(intent.x,angle.x,goal.x,dt*13.f);
        axis(intent.y,angle.y,goal.y,dt*11.f);
    }

    Vec value() const { return angle; }

private:
    Vec intent,angle;

    // Exact step of two cascaded exponential filters. The view starts gently
    // from rest and converges without overshoot when the cursor holds still.
    static void axis(float& intent,float& angle,float goal,float time) {
        const float decay=std::exp(-time);
        angle=goal+((angle-goal)+(intent-goal)*time)*decay;
        intent=goal+(intent-goal)*decay;
    }
};

struct CameraView {
    Vec tilt{};
    Vec focus{width*.5f,height*.5f};
    float zoom=1.f;
    bool tiltEnabled=false;

    Vec perspective() const { return {.023f*tilt.x,.015f*tilt.y}; }
    // Reserve the same edge crop for every pointer position, including neutral.
    float overscan() const { return tiltEnabled?1.055f:1.f; }
    float roll() const { return .006f*tilt.x; }

    Vec sample(Vec screen) const {
        const Vec q{screen.x/(width*.5f)-1,screen.y/(height*.5f)-1};
        const Vec p=rotate(q,roll())*(1.f/(overscan()+dot(q,perspective())));
        return focus+(pixels(p)-focus)*zoom;
    }

    Vec display(Vec scene) const {
        const Vec p=focus+(scene-focus)*(1.f/zoom);
        const Vec rotated=rotate({p.x/(width*.5f)-1,p.y/(height*.5f)-1},-roll());
        return pixels(rotated*(overscan()/(1.f-dot(rotated,perspective()))));
    }

private:
    static Vec pixels(Vec p) { return {(p.x+1)*(width*.5f),(p.y+1)*(height*.5f)}; }
    static Vec rotate(Vec v,float angle) {
        const float c=std::cos(angle),s=std::sin(angle);
        // Rotation in pixels, expressed in normalized coordinates for the 2:1 viewport.
        return {v.x*c-v.y*s*.5f,v.x*s*2.f+v.y*c};
    }
};
}
