#pragma once
#include "core/math.hpp"

namespace vl {
enum class EnemyKind {drone,ranger,turret,stalker,boss};
enum class SupplyKind {ammo,salvage,health};
struct Character {
    const char *name,*role,*art,*color,*trait,*description;
    int health; float speed,jump,ammo;
};
struct Difficulty { const char* name; float hp,damage,pace,score; };
inline constexpr size_t weaponCount = 7, upgradeCount = 8, perkCount = 6;
struct Weapon {
    const char *name,*shortName,*type,*color,*description;
    int damage,magazine,reserve; float reload,interval,speed,range,spread;
    int pellets,unlock,pierce;
    float blast=0,gravity=0,cost=1;
};
struct Upgrade { const char *key,*name,*description; int base; };
// Operative upgrades apply to every weapon and persist with the campaign.
struct Perk { const char *key,*name,*description; int base,maxRank; };
struct Lift { float x,y,w,dx,dy,period,phase; };
struct Vent { float x,y,phase; };
struct EnemyPlacement { EnemyKind kind; float x,y,patrol; };
struct SupplyPlacement { SupplyKind kind; float x,y; };
struct Stage {
    const char *name,*art,*location,*threat,*accent,*cold,*briefing,*boss,*pit;
    float worldWidth; Vec beacon,exit;
    std::vector<Rect> ground,ledges;
    std::vector<Lift> lifts; std::vector<Vent> vents;
    std::vector<EnemyPlacement> enemies; std::vector<SupplyPlacement> supplies;
};
struct Record { float time=0; int score=0; };
struct Save {
    int cleared=0,score=0,credits=0,character=0,difficulty=1,equipped=0;
    int64_t updated=0;
    std::array<std::array<int,upgradeCount>,weaponCount> tiers{};
    std::array<int,perkCount> perks{};
    std::array<Record,5> records{};
};
// Keyboard remapping slots. Every action has keysPerAction slots. A slot holds a scancode, 0 for unassigned or -1 for the default.
inline constexpr size_t keysPerAction = 3, actionSlots = 24, keymapSize = keysPerAction * actionSlots;
enum class SprintMode { hold, toggle, automatic };
struct Settings {
    std::array<int,keymapSize> keymap = [] { std::array<int,keymapSize> v{}; v.fill(-1); return v; }();
    int sprintMode = 0;
    bool crouchToggle = false, diagnostics = false;
    bool audio=true,motion=true,cinematic=true,high=true;
    float music=.74f,effects=.68f; int difficulty=1;
};
} // namespace vl
