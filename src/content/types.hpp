#pragma once
#include "core/math.hpp"

namespace vl {
enum class EnemyKind {drone,ranger,turret,stalker,boss,scavenger,sniper,bulwark,interceptor,juggernaut};
enum class Biome {mine,water,biodome,forge,orbital};
inline constexpr int sectorCount=20;
inline constexpr int cooperativeSectorCount=6;
enum class CoopMode : int { campaign=0, cooperative=1 };
enum class SupplyKind {ammo,salvage,health};
struct Character {
    const char *name,*role,*art,*color,*trait,*description;
    int health; float speed,jump,ammo;
    int rig=-1; // 0..5 selects the original articulated armor silhouettes; -1 uses the default.
};
struct Difficulty { const char* name; float hp,damage,pace,score; };
inline constexpr size_t weaponCount = 7, upgradeCount = 10, perkCount = 6;
struct Weapon {
    const char *name,*shortName,*type,*color,*description;
    int damage,magazine,reserve; float reload,interval,speed,range,spread;
    int pellets,unlock,pierce;
    float blast=0,gravity=0,cost=1;
};
struct Upgrade { const char *key,*name,*description; int base; };
// Operative upgrades apply to every weapon and persist with the campaign.
struct Perk { const char *key,*name,*description; int base,maxRank; };
struct SpecialUpgrade { const char *name,*description; };
struct CharacterSkill { const char *key,*name,*description; int base,maxRank; };
// A sloped, walkable staircase between two footholds, left endpoint first.
// Characters and ground enemies travel on its surface using normal left/right controls.
struct Stair { Vec from,to; };
struct Lift { float x,y,w,dx,dy,period,phase; };
struct Vent { float x,y,phase; };
struct EnemyPlacement { EnemyKind kind; float x,y,patrol; bool elite=false; };
struct SupplyPlacement { SupplyKind kind; float x,y; };
enum class CrateKind {charge,fuel,reactor};
struct CrateSpec { const char* name; int hits; float radius,damage,w,h; uint32_t color; };
inline constexpr CrateSpec crateSpecs[]{
    {"Charge box",2,100,48,32,30,0xffd55b},
    {"Fuel crate",4,155,87,40,33,0xff8e51},
    {"Reactor cell",7,218,141,45,38,0xff557e}
};
inline const CrateSpec& crateSpec(CrateKind kind) { return crateSpecs[static_cast<int>(kind)]; }
struct CratePlacement { CrateKind kind; Vec base; };
// Alternating upper/lower route gates. The other operative must hold the relay pad.
enum class RelayControl { pressurePlate, lever };
struct RelayGate {
    Rect barrier;     // Frame footprint; shutter physically closes this passage
    Vec pad;          // Plate / wall-console activation position
    int operatorPlayer=0;
    RelayControl control=RelayControl::pressurePlate;
};
// Optional authored outline around an existing rectangular structure. This
// controls its visual silhouette; the underlying rectangular collision remains
// authoritative until polygon collider support is added.
struct StructurePolygon {
    int layer=0; // 0 ground, 1 ledges, 2 cover, 3 relay doors
    int index=0;
    std::vector<Vec> corners;
};
struct Stage {
    const char *name,*art,*location,*threat,*accent,*cold,*briefing,*boss;
    Biome biome;
    float worldWidth; Vec exit;
    std::vector<Vec> beacons;
    std::vector<Rect> ground,ledges;
    std::vector<Lift> lifts; std::vector<Vent> vents;
    std::vector<EnemyPlacement> enemies; std::vector<SupplyPlacement> supplies;
    std::vector<Rect> cover{};
    std::vector<CratePlacement> crates{};
    std::vector<RelayGate> relays{};
    std::vector<Stair> stairs{};
    std::vector<StructurePolygon> structurePolygons{};
    Vec insertion{}; // Zero y: initial floor elevation. Editable per-level operative spawn.
};
struct Record { float time=0; int score=0; };
// Remaining lives are mission-local; purchased extra life slots are saved per campaign.
inline constexpr int maxExtraLives=4;
inline constexpr int startingLives(int difficulty,int extra=0) {
    constexpr int byDifficulty[]{6,4,2}; // Recruit / Standard / Veteran
    return byDifficulty[std::clamp(difficulty,0,2)]+std::clamp(extra,0,maxExtraLives);
}
inline constexpr int extraLifePrice(int purchased) { return 125+75*std::clamp(purchased,0,maxExtraLives); }
struct Save {
    int cleared=0,score=0,credits=0,character=0,difficulty=1,equipped=3;
    int extraLives=0; // Permanent capacity; does not change on death.
    // Buying a weapon, not clearing its unlock sector, grants access.
    std::array<bool,weaponCount> owned=[] { std::array<bool,weaponCount> result{}; result[3]=true; return result; }();
    std::array<std::array<int,2>,6> skills{};
    int64_t updated=0;
    std::array<std::array<int,upgradeCount>,weaponCount> tiers{};
    std::array<int,perkCount> perks{};
    std::array<Record,sectorCount> records{};
};
// Keyboard remapping slots. Every action has keysPerAction slots. A slot holds a scancode, 0 for unassigned or -1 for the default.
inline constexpr size_t keysPerAction = 3, actionSlots = 24, keymapSize = keysPerAction * actionSlots;
enum class SprintMode { hold, toggle, automatic };
// Rendering complexity is local presentation state. It never enters network packets or save-game simulation.
// Levels: 0 = off/minimal, 1 = low, 2 = high, 3 = ultra.
struct GraphicsQuality {
    int preset=2; // 0 performance, 1 balanced, 2 quality, 3 ultra, 4 custom
    int resolution=3; // 0 50%, 1 67%, 2 80%, 3 100%, 4 115%
    bool dynamicResolution=true;
    int lighting=2,shadows=2,bloom=2,glow=2,fog=2,reflections=2;
    int particles=2,scenery=2,materials=2,shafts=2,water=2,antialias=2,grading=2;
};
inline GraphicsQuality graphicsPreset(int preset) {
    GraphicsQuality q;
    q.preset=std::clamp(preset,0,3);
    switch (q.preset) {
    case 0:
        q.resolution=1; q.lighting=0; q.shadows=0; q.bloom=0; q.glow=0; q.fog=0;
        q.reflections=0; q.particles=0; q.scenery=0; q.materials=0; q.shafts=0;
        q.water=0; q.antialias=0; q.grading=0; break;
    case 1:
        q.resolution=2; q.lighting=1; q.shadows=1; q.bloom=1; q.glow=1; q.fog=1;
        q.reflections=1; q.particles=1; q.scenery=1; q.materials=1; q.shafts=1;
        q.water=1; q.antialias=1; q.grading=1; break;
    case 3:
        q.resolution=4; q.lighting=3; q.shadows=3; q.bloom=3; q.glow=3; q.fog=3;
        q.reflections=3; q.particles=3; q.scenery=3; q.materials=3; q.shafts=3;
        q.water=3; q.antialias=2; q.grading=3; q.dynamicResolution=false; break;
    default: break;
    }
    return q;
}

struct Settings {
    std::array<int,keymapSize> keymap = [] { std::array<int,keymapSize> v{}; v.fill(-1); return v; }();
    int sprintMode = 0;
    bool crouchToggle = false, diagnostics = false, debugUnlockLevels = false;
    bool audio=true,motion=true,cinematic=true,high=true,fpsCounter=false;
    int fpsCorner=1; // 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right
    GraphicsQuality graphics{};
    float music=.74f,effects=.68f; int difficulty=1;
};
} // namespace vl
