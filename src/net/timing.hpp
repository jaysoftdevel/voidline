#pragma once
#include <chrono>
#include <cstdint>

namespace vl::net {
using Millis=uint64_t;
inline constexpr float snapshotInterval=1.f/60;
inline constexpr float inputInterval=1.f/120;
inline constexpr Millis openingDuration=10500;
inline constexpr Millis transitionDuration=1250;
inline constexpr Millis preparationTimeout=12000;

inline Millis now() {
    return static_cast<Millis>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

class HostClock {
public:
    bool sample(Millis sent,Millis received,Millis host);
    bool ready() const { return bestRoundTrip!=invalid; }
    Millis estimate(Millis local) const;
    Millis lead() const;
private:
    static constexpr Millis invalid=~Millis(0);
    Millis bestRoundTrip=invalid;
    int64_t offset=0;
};
}
