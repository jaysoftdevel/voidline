#pragma once
// Tuning constants and rule tables shared by the simulation sources.
#include "game/game.hpp"
#include <limits>

namespace vl {
inline constexpr float infinity=std::numeric_limits<float>::infinity();
constexpr float gravity=1510, playerW=32, playerH=56, crouchHeight=39;
constexpr float windupDuration=.032f;
struct GunSound { const char* name; float rate,gain; };
constexpr GunSound gunSounds[weaponCount]{{"carbine",1,1},{"shotgun",1,1},{"arc",1,1},{"pistol",1,1},
    {"pistol",1.32f,.8f},{"shotgun",.58f,1.05f},{"arc",.62f,1.1f}};
constexpr const char* footSounds[]{"step-rock","step-grate","step-rock","step-deck","step-deck"};
struct EnemySpec { float w,h,speed,reach,delay; int hp,damage,score; };
constexpr EnemySpec specs[]{
    {48,30,78,390,1.65f,57,10,14}, {35,54,72,435,1.34f,82,13,20},
    {48,46,0,490,1.52f,102,14,27}, {54,37,176,330,1.65f,93,17,25},
    {108,97,67,600,1.30f,420,16,135}
};
struct Pattern { int fans; float spread,velocity,interval; uint32_t color; };
constexpr Pattern patterns[]{
    {3,.33f,320,1.29f,0xffa55e}, {5,.64f,295,1.48f,0x7ef1e9}, {3,.39f,352,1.09f,0xbaff9d},
    {5,.73f,378,1.27f,0xff905d}, {5,.58f,440,1.17f,0xa9c9ff}
};
inline const EnemySpec& spec(EnemyKind kind) { return specs[static_cast<int>(kind)]; }


}
