// Campaign content: characters, difficulties, weapons, upgrades and the five sector layouts.
#pragma once
#include "data.hpp"
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
inline const std::array<Weapon, 4> weapons{{
    {"Field Carbine", "Carbine", "AUTOMATIC", "#ffad65", "Steady automatic fire at medium range.", 25, 18, 126, 0.95, 0.145, 1030, 760, 0.026, 1, 0, 0},
    {"Breach Shotgun", "Shotgun", "SCATTER", "#ff8f7d", "Six heavy pellets in a close-range spread.", 17, 7, 49, 1.27, 0.53, 955, 365, 0.38, 6, 1, 0},
    {"Arc Rifle", "Arc Rifle", "PRECISION", "#72f1ff", "Fast energy bolts that pierce a second target.", 54, 9, 54, 1.15, 0.285, 1460, 970, 0.008, 1, 2, 1},
    {"Service Pistol", "Pistol", "SIDEARM", "#ece1bb", "Emergency sidearm with unlimited reserve.", 14, 10, -1, 0.78, 0.24, 960, 680, 0.037, 1, 0, 0}
}};
inline const std::array<Upgrade, 4> upgrades{{
    {"damage", "Damage", "+20% base damage per tier", 36},
    {"magazine", "Magazine", "+25% magazine capacity per tier", 28},
    {"reload", "Reload speed", "15% less reload time per tier", 30},
    {"reserve", "Ammo reserve", "+30% reserve ammo per tier", 24}
}};
inline const std::array<Stage, 5> stages{{
    {"The Descent", "descent", "Abandoned mine", "LOW", "#ff9c58", "#58bbca", "Cross the mining gantries and take the cargo lift out of the quarry.", "Mule-7", "void", 3350, {1600, 495}, {3245, 490},
     {{0, 505, 580, 110}, {675, 505, 600, 110}, {1370, 495, 600, 110}, {2090, 510, 530, 110}, {2730, 490, 620, 110}},
     {{270, 410, 155, 17}, {496, 376, 136, 17}, {740, 403, 145, 17}, {995, 374, 154, 17}, {1200, 437, 124, 17}, {1455, 394, 150, 17}, {1690, 360, 158, 17}, {1900, 420, 152, 17}, {2170, 409, 157, 17}, {2410, 369, 145, 17}, {2622, 446, 134, 17}, {2825, 390, 140, 17}},
     {},
     {},
     {{EnemyKind::drone, 438, 336, 0}, {EnemyKind::ranger, 825, 505, 115}, {EnemyKind::drone, 1110, 306, 0}, {EnemyKind::ranger, 1518, 495, 105}, {EnemyKind::drone, 1800, 290, 0}, {EnemyKind::ranger, 2285, 510, 100}, {EnemyKind::turret, 2483, 369, 0}, {EnemyKind::drone, 2770, 353, 0}, {EnemyKind::boss, 3050, 490, 0}},
     {{SupplyKind::ammo, 240, 505}, {SupplyKind::salvage, 336, 410}, {SupplyKind::ammo, 808, 403}, {SupplyKind::health, 1150, 505}, {SupplyKind::salvage, 1060, 374}, {SupplyKind::ammo, 1730, 495}, {SupplyKind::health, 2240, 409}, {SupplyKind::ammo, 2575, 510}, {SupplyKind::salvage, 2478, 369}, {SupplyKind::health, 2820, 490}}},
    {"Tideworks", "tideworks", "Flooded utilities", "MODERATE", "#71f2e5", "#2ca0bc", "Ride the pump lifts over the electrified floodwater and secure the spillway.", "Sluice Warden", "water", 3630, {1920, 480}, {3525, 490},
     {{0, 510, 430, 110}, {600, 495, 400, 110}, {1220, 510, 520, 110}, {1760, 480, 470, 110}, {2390, 510, 550, 110}, {3060, 490, 570, 110}},
     {{198, 412, 146, 17}, {460, 446, 104, 17}, {688, 381, 163, 17}, {905, 420, 94, 17}, {1100, 420, 130, 17}, {1320, 414, 139, 17}, {1570, 386, 148, 17}, {1810, 374, 155, 17}, {2110, 391, 121, 17}, {2260, 426, 126, 17}, {2430, 377, 153, 17}, {2710, 415, 166, 17}, {2960, 433, 130, 17}, {3100, 386, 131, 17}},
     {{1048, 440, 94, 0, 36, 2.2, 0}, {2340, 431, 90, 30, 32, 2.7, 1.3}},
     {{840, 495, 0.5}, {2585, 510, 1.5}},
     {{EnemyKind::ranger, 315, 510, 80}, {EnemyKind::drone, 700, 315, 0}, {EnemyKind::turret, 920, 420, 0}, {EnemyKind::ranger, 1440, 510, 120}, {EnemyKind::drone, 1580, 323, 0}, {EnemyKind::stalker, 1930, 480, 112}, {EnemyKind::ranger, 2185, 480, 88}, {EnemyKind::turret, 2473, 377, 0}, {EnemyKind::drone, 2770, 342, 0}, {EnemyKind::ranger, 2830, 510, 77}, {EnemyKind::boss, 3360, 490, 0}},
     {{SupplyKind::ammo, 165, 510}, {SupplyKind::salvage, 275, 412}, {SupplyKind::health, 725, 495}, {SupplyKind::salvage, 1150, 420}, {SupplyKind::ammo, 1370, 414}, {SupplyKind::health, 1680, 510}, {SupplyKind::ammo, 1990, 480}, {SupplyKind::salvage, 2180, 391}, {SupplyKind::health, 2580, 377}, {SupplyKind::ammo, 2830, 415}, {SupplyKind::health, 3110, 490}}},
    {"Overgrowth", "overgrowth", "Research biodome", "ELEVATED", "#b0ef8b", "#bb69dc", "Navigate the collapsed biodome and shut down the colony’s overgrown defense core.", "Bloom Core", "spores", 3810, {1930, 495}, {3705, 485},
     {{0, 510, 480, 110}, {600, 500, 475, 110}, {1205, 510, 470, 110}, {1820, 495, 520, 110}, {2470, 510, 550, 110}, {3180, 485, 630, 110}},
     {{199, 411, 150, 17}, {430, 382, 129, 17}, {534, 445, 119, 17}, {709, 390, 150, 17}, {960, 379, 160, 17}, {1110, 442, 126, 17}, {1325, 411, 140, 17}, {1510, 362, 136, 17}, {1685, 437, 126, 17}, {1810, 388, 149, 17}, {2120, 382, 141, 17}, {2350, 425, 143, 17}, {2520, 391, 175, 17}, {2795, 363, 135, 17}, {3000, 419, 105, 17}, {3110, 371, 135, 17}, {3250, 398, 143, 17}},
     {{1710, 433, 105, 30, 25, 2.3, 1}},
     {{693, 500, 0}, {1542, 510, 1.25}, {2850, 510, 2}},
     {{EnemyKind::stalker, 357, 510, 95}, {EnemyKind::drone, 584, 321, 0}, {EnemyKind::turret, 800, 390, 0}, {EnemyKind::stalker, 920, 500, 130}, {EnemyKind::drone, 1250, 328, 0}, {EnemyKind::stalker, 1510, 510, 130}, {EnemyKind::ranger, 1940, 495, 130}, {EnemyKind::drone, 2050, 313, 0}, {EnemyKind::turret, 2200, 382, 0}, {EnemyKind::stalker, 2535, 510, 135}, {EnemyKind::ranger, 2900, 510, 95}, {EnemyKind::drone, 3100, 303, 0}, {EnemyKind::boss, 3520, 485, 0}},
     {{SupplyKind::ammo, 210, 510}, {SupplyKind::salvage, 483, 382}, {SupplyKind::health, 670, 500}, {SupplyKind::ammo, 782, 390}, {SupplyKind::salvage, 1119, 442}, {SupplyKind::health, 1410, 411}, {SupplyKind::ammo, 1540, 510}, {SupplyKind::salvage, 1705, 437}, {SupplyKind::health, 2180, 495}, {SupplyKind::ammo, 2530, 510}, {SupplyKind::salvage, 2840, 363}, {SupplyKind::health, 3245, 485}}},
    {"Iron Foundry", "foundry", "Thermal reactor", "SEVERE", "#ff8454", "#ffbc73", "Cross the forge over molten iron and destroy the reactor sentinel.", "Crucible", "lava", 4120, {2000, 470}, {4015, 485},
     {{0, 515, 465, 110}, {622, 490, 400, 110}, {1195, 515, 525, 110}, {1890, 470, 520, 110}, {2540, 510, 550, 110}, {3270, 485, 850, 110}},
     {{198, 405, 132, 17}, {409, 386, 120, 17}, {503, 438, 93, 17}, {668, 392, 152, 17}, {928, 385, 121, 17}, {1080, 431, 116, 17}, {1260, 403, 149, 17}, {1515, 382, 130, 17}, {1714, 443, 110, 17}, {1855, 395, 115, 17}, {2080, 356, 144, 17}, {2330, 376, 140, 17}, {2450, 425, 131, 17}, {2640, 410, 150, 17}, {2870, 366, 161, 17}, {3102, 431, 103, 17}, {3215, 385, 136, 17}, {3415, 372, 142, 17}},
     {{1765, 444, 104, 23, 48, 2.7, 0}, {3146, 427, 112, 29, 35, 2.35, 0.5}},
     {{754, 490, 0}, {1420, 515, 1.3}, {2270, 470, 1}, {2710, 510, 2.35}, {3200, 385, 0.2}},
     {{EnemyKind::ranger, 315, 515, 100}, {EnemyKind::drone, 565, 337, 0}, {EnemyKind::turret, 730, 392, 0}, {EnemyKind::stalker, 901, 490, 88}, {EnemyKind::drone, 1190, 323, 0}, {EnemyKind::ranger, 1390, 515, 110}, {EnemyKind::turret, 1600, 382, 0}, {EnemyKind::drone, 1780, 311, 0}, {EnemyKind::stalker, 2140, 470, 108}, {EnemyKind::ranger, 2280, 470, 115}, {EnemyKind::drone, 2590, 315, 0}, {EnemyKind::turret, 2770, 410, 0}, {EnemyKind::stalker, 2910, 510, 122}, {EnemyKind::drone, 3200, 304, 0}, {EnemyKind::ranger, 3475, 485, 102}, {EnemyKind::boss, 3795, 485, 0}},
     {{SupplyKind::ammo, 180, 515}, {SupplyKind::salvage, 482, 386}, {SupplyKind::health, 830, 490}, {SupplyKind::ammo, 950, 385}, {SupplyKind::salvage, 1130, 431}, {SupplyKind::health, 1480, 515}, {SupplyKind::ammo, 1600, 515}, {SupplyKind::health, 2200, 356}, {SupplyKind::ammo, 2640, 510}, {SupplyKind::salvage, 2980, 366}, {SupplyKind::health, 3340, 485}, {SupplyKind::ammo, 3565, 485}}},
    {"Terminus", "terminus", "Orbital uplink", "CRITICAL", "#b7b8ff", "#7ed4ff", "Scale the orbital relay and take down the station’s last guardian.", "Helix Prime", "storm", 4470, {2260, 485}, {4370, 470},
     {{0, 508, 460, 110}, {620, 476, 415, 110}, {1190, 510, 500, 110}, {1870, 485, 610, 110}, {2660, 504, 550, 110}, {3390, 470, 1080, 110}},
     {{178, 410, 158, 17}, {398, 358, 122, 17}, {520, 416, 109, 17}, {675, 369, 146, 17}, {917, 350, 148, 17}, {1050, 419, 152, 17}, {1265, 408, 157, 17}, {1538, 382, 132, 17}, {1705, 430, 108, 17}, {1810, 372, 150, 17}, {1980, 365, 155, 17}, {2210, 366, 139, 17}, {2402, 397, 140, 17}, {2545, 440, 128, 17}, {2715, 386, 160, 17}, {2940, 365, 164, 17}, {3124, 413, 106, 17}, {3250, 399, 150, 17}, {3490, 361, 155, 17}, {3710, 380, 145, 17}},
     {{1760, 429, 114, 32, 38, 2.6, 0.4}, {2540, 432, 112, 34, 46, 3.1, 1.3}, {3270, 426, 108, 36, 37, 2.3, 0}},
     {{802, 476, 0.1}, {1430, 510, 0.8}, {2350, 485, 2.7}, {2920, 504, 1.3}, {3580, 470, 2}},
     {{EnemyKind::ranger, 312, 508, 106}, {EnemyKind::drone, 550, 321, 0}, {EnemyKind::turret, 750, 369, 0}, {EnemyKind::stalker, 895, 476, 130}, {EnemyKind::drone, 1130, 318, 0}, {EnemyKind::ranger, 1380, 510, 110}, {EnemyKind::turret, 1600, 382, 0}, {EnemyKind::drone, 1760, 325, 0}, {EnemyKind::ranger, 2070, 485, 132}, {EnemyKind::stalker, 2350, 485, 110}, {EnemyKind::drone, 2550, 331, 0}, {EnemyKind::turret, 2775, 386, 0}, {EnemyKind::stalker, 3050, 504, 132}, {EnemyKind::drone, 3330, 306, 0}, {EnemyKind::ranger, 3560, 470, 103}, {EnemyKind::turret, 3730, 380, 0}, {EnemyKind::boss, 4030, 470, 0}},
     {{SupplyKind::ammo, 220, 508}, {SupplyKind::salvage, 473, 358}, {SupplyKind::health, 865, 476}, {SupplyKind::ammo, 1010, 350}, {SupplyKind::salvage, 1140, 419}, {SupplyKind::health, 1320, 408}, {SupplyKind::ammo, 1570, 510}, {SupplyKind::salvage, 1835, 372}, {SupplyKind::health, 2240, 366}, {SupplyKind::ammo, 2678, 504}, {SupplyKind::salvage, 3010, 365}, {SupplyKind::health, 3425, 470}, {SupplyKind::ammo, 3740, 470}, {SupplyKind::health, 3825, 380}}}
}};
}
