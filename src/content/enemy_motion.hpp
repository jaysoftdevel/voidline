#pragma once
#include "content/skeleton_types.hpp"
#include <array>
namespace vl {
// Optional motion rig, loaded once from assets/models/enemies.vlem in game.vpak.
inline std::array<SkeletonProfile,10> enemySkeletons{};
}
