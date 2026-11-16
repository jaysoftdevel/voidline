#pragma once
#include <algorithm>
#include <cmath>

namespace vl::boss_tactics {
struct Intent {float desiredSpeed=0, lift=0, attackInterval=1, telegraph=0.38f; bool attack=false;};
// One deterministic AI per guardian. All timings depend on simulation time, not
// frame rates or unsynchronized random state; safe in multiplayer replays.
inline Intent decide(float seconds,float healthFraction,int phase,float distance,
                     float speed,float pace,bool hasLineOfSight) {
    const float aggression=1.f+phase*.26f+(1.f-std::clamp(healthFraction,0.f,1.f))*.25f;
    const float pulse=std::sin(seconds*(1.48f+phase*.25f));
    const float pressure=distance>215.f?1.f:distance<115.f?-.80f:0.f;
    // Weave and switch direction every few beats, rather than acting as a static turret.
    // Periodic step-away is signaled by negative speed; the caller applies facing.
    const float weave=std::abs(pulse)>.72f?(pulse>0.f?.62f:-.53f):0.f;
    const float advance=pressure!=0.f?pressure:weave;
    Intent out;
    out.desiredSpeed=advance*speed*pace*aggression;
    const float beat=std::sin(seconds*(2.0f+phase*.31f));
    out.lift=(phase==0?4.f:7.f+phase*6.f)*std::max(0.f,beat);
    out.attackInterval=std::max(.36f,1.0f/(aggression*(phase>0?1.12f:1.f)));
    out.telegraph=std::max(.17f,.38f/(1.f+phase*.24f));
    out.attack=hasLineOfSight;
    return out;
}
} // namespace vl::boss_tactics
