#pragma once
#include "core/math.hpp"

namespace vl {
// Entry fraction of the segment p + d*t against a box, or infinity when it misses.
float segment(Vec p,Vec d,Rect r);
// Two-bone solver. The joint is placed on the side selected by forward (+1 or -1).
Vec solveKnee(Vec hip,Vec ankle,float thigh,float shin,int forward);
// Keep the rendered ankle inside the reachable annulus of a two-bone leg.
// IK alone cannot fix an overextended leg: without this constraint the knee
// solves for a *different* endpoint while the shin still draws to the old one.
Vec reachableAnkle(Vec hip,Vec target,float thigh,float shin);
}
