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
    {"crit", "Critical hits", "+7% chance for 2x damage per tier", 40},
    {"specialA", "Signature I", "Weapon-specific specialization", 65},
    {"specialB", "Signature II", "Weapon-specific specialization", 80}
}};
// The final two upgrade lanes are unique to every gun, not generic stat bonuses.
inline const std::array<std::array<SpecialUpgrade,2>,weaponCount> weaponSpecials{{
    {{{"Recoil cage","12% less recoil spread per rank"},{"Overclock","5% faster carbine cycle per rank"}}},
    {{{"Extra buck","+1 pellet per rank"},{"Breaching powder","+16% damage at close range per rank"}}},
    {{{"Arc conductor","+1 enemy pierced per rank"},{"Ion amplifier","+14% energy damage per rank"}}},
    {{{"Quickdraw","8% faster pistol fire per rank"},{"Lucky chamber","+7% critical chance per rank"}}},
    {{{"Drum spring","+12% magazine size per rank"},{"Blitz actuator","+7% rate of fire per rank"}}},
    {{{"Wide detonation","+15% blast radius per rank"},{"Heavy payload","+12% blast damage per rank"}}},
    {{{"Sabot core","+1 enemy pierced per rank"},{"Rail capacitor","+18% slug damage per rank"}}}
}};
// Each operative has two individual upgradeable abilities, saved per character.
inline const std::array<std::array<CharacterSkill,2>,6> characterSkills{{
    {{{"vossGuard","Iron resolve","-5% incoming damage per rank",75,3},{"vossAssault","Point assault","+8% close-range damage per rank",82,3}}},
    {{{"rookParkour","Ghost step","+6% run speed and +5% jump per rank",70,3},{"rookAir","Air ace","+9% damage while airborne per rank",76,3}}},
    {{{"valeRig","Field loader","+12% ammunition capacity per rank",68,3},{"valeTuning","Quick service","8% shorter reload per rank",76,3}}},
    {{{"mercerBreach","Close quarters","+11% close-range damage per rank",78,3},{"mercerPlate","Blast shielding","-7% damage taken per rank",72,3}}},
    {{{"coleSight","Hawkeye","+8% effective weapon range per rank",72,3},{"coleCalm","Still aim","+12% accuracy per rank",75,3}}},
    {{{"nyxEnergy","Energy flux","+12% arc and explosive damage per rank",82,3},{"nyxFocus","Flux reserve","+15% bullet-time recovery per rank",78,3}}}
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
