#pragma once
#include "core/math.hpp"

namespace vl {
// Entry fraction of the segment p + d*t against a box, or infinity when it misses.
float segment(Vec p,Vec d,Rect r);
// Two-bone solver. The joint is placed on the side selected by forward (+1 or -1).
Vec solveKnee(Vec hip,Vec ankle,float thigh,float shin,int forward);
}
