#pragma once
#include "game/entities.hpp"

namespace vl {
using KeyMask=uint16_t;
constexpr KeyMask keyBit(Key key) { return KeyMask(1u<<static_cast<unsigned>(key)); }

enum class AimMode : uint8_t { automatic,pointer,stick };
struct Controls {
    KeyMask held=0,pressed=0;
    int weapon=-1,cycle=0;
    AimMode aimMode=AimMode::automatic;
    Vec aim;
};

struct Operative {
    Player player;
    std::array<Gun,weaponCount> guns{};
    const Character* character=nullptr;
    int gun=0,maxHealth=100;
    float camera=0;
};

struct OperativeInput {
    std::array<bool,static_cast<size_t>(Key::count)> keys{};
    Vec mouse,stickAim;
    bool pointerActive=false,stickActive=false;
};
}
