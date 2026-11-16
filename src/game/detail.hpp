#pragma once
// Tuning constants and rule tables shared by the simulation sources.
#include "game/game.hpp"
#include "content/model_io.hpp"
#include <limits>

namespace vl {
inline constexpr float infinity=std::numeric_limits<float>::infinity();
constexpr float gravity=1510, playerW=32, playerH=56, crouchHeight=39;
constexpr float windupDuration=.032f;
struct GunSound { const char* name; float rate,gain; };
// Own samples for each weapon; do not pitch-shift pistol/shotgun/arc to
// impersonate the SMG, launcher or railgun.
constexpr GunSound gunSounds[weaponCount]{{"carbine",1,1},{"shotgun",1,1},{"arc",1,1},{"pistol",1,1},
    {"smg",1,.84f},{"launcher",1,1.04f},{"railgun",1,1.06f}};
// Physical material takes precedence over biome ambience: lifts and deck
// ledges are metal, cover is hard concrete unless part of a metal facility.
// Natural ground varies by the stage's authored environment.
inline const char* impactSound(const Platform& surface,Biome biome) {
    if (surface.lift || (!surface.ground && !surface.cover)) return "hit-metal";
    if (surface.cover) return (biome==Biome::forge || biome==Biome::orbital)?"hit-metal":"hit-concrete";
    switch (biome) {
    case Biome::mine: return "hit-rock";
    case Biome::water:
    case Biome::biodome: return "hit-concrete";
    case Biome::forge:
    case Biome::orbital: return "hit-metal";
    }
    return "hit-concrete";
}

constexpr const char* footSounds[]{"boot-rock","boot-grate","boot-rock","boot-deck","boot-deck"};
struct Pattern { int fans; float spread,velocity,interval; uint32_t color; };
constexpr Pattern patterns[]{
    {3,.33f,320,1.29f,0xffa55e}, {5,.64f,295,1.48f,0x7ef1e9}, {3,.39f,352,1.09f,0xbaff9d},
    {5,.73f,378,1.27f,0xff905d}, {5,.58f,440,1.17f,0xa9c9ff}
};



}
