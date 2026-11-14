#pragma once
#include <array>
#include "content/types.hpp"
// Unified side-view sprite placement. UV coordinates locate hand contacts on each image;
// muzzleV locates the barrel exit, so recoil/flash stay aligned for all seven weapons.
namespace vl::figures {
struct GunArt {
    const char* name;
    float length,aspect,muzzleV,gripU,gripV,foreU,foreV;
};
inline constexpr std::array<GunArt,weaponCount> gunArt{{
    {"carbine", 58,.353f,.39f,.29f,.68f,.70f,.56f},
    {"shotgun", 62,.314f,.36f,.27f,.76f,.70f,.52f},
    {"arc",     70,.231f,.39f,.30f,.80f,.65f,.55f},
    {"pistol",  28.5f,320.f/580.f,.39f,.39f,.71f,.56f,.58f},
    {"smg",     43.5f,320.f/740.f,.52f,.45f,.75f,.72f,.64f},
    {"launcher",53.f,340.f/850.f,.47f,.35f,.78f,.71f,.67f},
    {"railgun", 81.f,275.f/920.f,.49f,.34f,.75f,.61f,.64f}
}};
} // namespace vl::figures
