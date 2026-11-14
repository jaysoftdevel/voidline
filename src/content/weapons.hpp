#pragma once
#include "content/types.hpp"
namespace vl {
inline const std::array<Weapon, weaponCount> weapons{{
    {"Field Carbine", "Carbine", "AUTOMATIC", "#ffad65", "Steady automatic fire with long, accurate reach.", 25, 18, 126, 0.95, 0.145, 1180, 1250, 0.024, 1, 0, 0},
    {"Breach Shotgun", "Shotgun", "SCATTER", "#ff8f7d", "Six heavy pellets that stay lethal across a room.", 17, 7, 49, 1.27, 0.53, 1050, 560, 0.36, 6, 1, 0},
    {"Arc Rifle", "Arc", "PRECISION", "#72f1ff", "Fast energy bolts that pierce a second target.", 54, 9, 54, 1.15, 0.285, 1700, 1550, 0.008, 1, 2, 1, 0, 0, 1.2},
    {"Service Pistol", "Pistol", "SIDEARM", "#ece1bb", "Reliable sidearm with unlimited reserve.", 14, 10, -1, 0.78, 0.24, 1050, 950, 0.034, 1, 0, 0, 0, 0, 0.8},
    {"Hornet SMG", "SMG", "RAPID FIRE", "#ffe36e", "Extreme fire rate. Shreds drones and stalkers.", 12, 32, 192, 1.05, 0.062, 1120, 880, 0.075, 1, 1, 0, 0, 0, 1.0},
    {"Ember Launcher", "Launcher", "EXPLOSIVE", "#ff6b4a", "Lobbed shells that burst and hurt everything nearby.", 74, 4, 20, 1.65, 0.78, 700, 1100, 0.012, 1, 3, 0, 105, 560, 1.35},
    {"Longshot Railgun", "Railgun", "HEAVY PIERCE", "#b9a4ff", "A hypersonic slug that punches through four targets.", 150, 5, 25, 1.75, 0.92, 3400, 2600, 0, 1, 4, 3, 0, 0, 1.55}
}};
}
