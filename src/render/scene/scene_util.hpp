#pragma once
// Deterministic noise and palette helpers for the scenery and ambient drawing.
#include "render/renderer.hpp"

namespace vl::scene {
inline float hash(int i) {
    uint32_t n=uint32_t(i)*747796405u+2891336453u;
    n=((n>>((n>>28u)+4u))^n)*277803737u;
    return ((n>>22u)^n)/4294967296.f;
}
inline Color metal(Biome biome) {
    constexpr uint32_t tones[]{0x596a72,0x516d75,0x5b6b60,0x80655d,0x586485};
    return hex(tones[static_cast<int>(biome)]);
}
inline Color supplyColor(SupplyKind k) {
    return hex(k==SupplyKind::health?0x9dedb4:k==SupplyKind::ammo?0xffc57b:0x89e6ec);
}
inline float hashf(float seed,int i) { return hash(int(seed*131.f)+i*7919); }
}
