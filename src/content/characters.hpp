#pragma once
#include "content/types.hpp"
namespace vl {
inline const std::array<Character, 3> characters{{
    {"Mara Voss", "Vanguard", "voss", "#ffa25e", "125 suit integrity", "Heavy armor for taking the fight up close.", 125, 350, 655, 1},
    {"Iona Rook", "Scout", "rook", "#76e5d7", "Faster movement and higher jumps", "A lighter suit built for quick traversal.", 100, 395, 708, 1},
    {"Kade Vale", "Engineer", "vale", "#b9adff", "+30% reserve ammo", "An expanded supply rig for sustained fire.", 110, 365, 670, 1.3}
}};
inline const std::array<Difficulty, 3> difficulties{{
    {"Recruit", 0.73, 0.66, 0.78, 0.8},
    {"Standard", 1, 1, 1, 1},
    {"Veteran", 1.32, 1.34, 1.24, 1.28}
}};
}
