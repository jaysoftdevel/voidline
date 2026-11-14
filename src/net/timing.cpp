#include "net/timing.hpp"
#include <algorithm>

namespace vl::net {
bool HostClock::sample(Millis sent,Millis received,Millis host) {
    if (received<sent || received-sent>preparationTimeout || host>=(Millis{1}<<62) ||
        received>=(Millis{1}<<62)) return false;
    const Millis roundTrip=received-sent;
    if (roundTrip<bestRoundTrip) {
        bestRoundTrip=roundTrip;
        offset=static_cast<int64_t>(host)-static_cast<int64_t>(sent+roundTrip/2);
    }
    return true;
}
Millis HostClock::estimate(Millis local) const {
    return static_cast<Millis>(std::max<int64_t>(0,static_cast<int64_t>(local)+offset));
}
Millis HostClock::lead() const {
    return ready()?std::clamp(bestRoundTrip*2+150,Millis{350},Millis{5000}):350;
}
}
