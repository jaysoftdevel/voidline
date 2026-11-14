#pragma once
// Sprite placement for the painted weapons. Lengths are in world pixels.
namespace vl::figures {
struct GunArt { const char* name; float length,aspect,muzzleV,gripU,gripV,foreU,foreV; };
inline constexpr GunArt gunArt[]{
    {"carbine",58,.353f,.39f,.29f,.68f,.70f,.56f},
    {"shotgun",62,.314f,.36f,.27f,.76f,.70f,.52f},
    {"arc",70,.231f,.39f,.30f,.80f,.65f,.55f}
};
}
