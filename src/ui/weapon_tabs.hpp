#pragma once
#include "content/weapons.hpp"

namespace vl::ui {
// Availability is distinct from ownership: a newly unlocked but unbought
// weapon needs an enabled tab so the player can inspect its purchase button.
inline bool weaponTabSelectable(int id,int cleared,bool debug) {
    return id>=0 && id<static_cast<int>(weaponCount) &&
        (debug || cleared>=weapons[static_cast<size_t>(id)].unlock);
}
}
