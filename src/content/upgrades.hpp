#pragma once
#include "content/types.hpp"
namespace vl {
inline const std::array<Upgrade, upgradeCount> upgrades{{
    {"damage", "Damage", "+20% base damage per tier", 36},
    {"magazine", "Magazine", "+25% magazine capacity per tier", 28},
    {"reload", "Reload speed", "15% less reload time per tier", 30},
    {"reserve", "Ammo reserve", "+30% reserve ammo per tier", 24},
    {"range", "Range", "+14% effective range per tier", 26},
    {"rate", "Fire rate", "8% shorter delay between shots per tier", 34},
    {"accuracy", "Accuracy", "22% tighter spread per tier", 26},
    {"crit", "Critical hits", "+7% chance for 2x damage per tier", 40}
}};
inline const std::array<Perk, perkCount> perks{{
    {"suit", "Reinforced suit", "+10% suit integrity per rank", 44, 4},
    {"frame", "Light frame", "+4% speed and +3% jump height per rank", 40, 4},
    {"focus", "Focus core", "+25% bullet-time reserve and faster refill", 46, 4},
    {"scavenger", "Scavenger", "+20% ammo from pickups, more salvage score", 38, 4},
    {"medic", "Field medic", "+15% healing from repair pickups", 36, 4},
    {"plating", "Ballistic plating", "6% less damage taken per rank", 52, 4}
}};
}
