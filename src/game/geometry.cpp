#include "game/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace vl {
namespace {
constexpr float infinity=std::numeric_limits<float>::infinity();
}
float segment(Vec p,Vec d,Rect r) {
    float near=0,far=1;
    const float origins[]{p.x,p.y}, deltas[]{d.x,d.y};
    const float lows[]{r.x,r.y}, highs[]{r.right(),r.bottom()};
    for (int axis=0;axis<2;++axis) {
        if (std::abs(deltas[axis])<.0001f) {
            if (origins[axis]<lows[axis] || origins[axis]>highs[axis]) return infinity;
            continue;
        }
        const float a=(lows[axis]-origins[axis])/deltas[axis];
        const float b=(highs[axis]-origins[axis])/deltas[axis];
        near=std::max(near,std::min(a,b));
        far=std::min(far,std::max(a,b));
        if (near>far) return infinity;
    }
    return near;
}

Vec solveKnee(Vec hip,Vec ankle,float thigh,float shin,int forward) {
    Vec delta=ankle-hip;
    const float d=clamp(length(delta),std::abs(thigh-shin)+.01f,thigh+shin-.01f);
    const Vec along=length(delta)>.001f ? normal(delta) : Vec{0,1};
    const float projection=(thigh*thigh-shin*shin+d*d)/(2*d);
    const float bend=std::sqrt(std::max(0.f,thigh*thigh-projection*projection));
    return hip+along*projection+Vec{along.y,-along.x}*(bend*forward);
}
} // namespace vl
