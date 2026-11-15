#pragma once
#include "content/characters.hpp"
#include "content/skeleton.hpp"
#include <filesystem>
#include <string>

namespace vl {
// The editable enemy dimensions and movement/damage rules. Each enemy archetype
// is independently serialized to models/enemies/NN-name.vlenemy.json.
struct EnemySpec {
    float w,h,speed,reach,delay;
    int hp,damage,score;
};
inline std::array<EnemySpec,10> specs{{
    {48,30,78,390,1.65f,57,10,14}, {35,54,72,435,1.34f,82,13,20},
    {48,46,0,490,1.52f,102,14,27}, {54,37,176,330,1.65f,93,17,25},
    {108,97,67,600,1.30f,420,16,135},
    {32,32,225,82,0,42,9,14}, {35,54,62,590,2.15f,90,18,43},
    {49,62,48,390,1.86f,250,22,65}, {56,33,108,440,.96f,116,17,52},
    {70,74,55,470,1.65f,365,27,108}
}};
// Enemy rigs use the same 14-joint XYZ and Base/Aim/Run authoring model as operatives.
// The game can consume the exported runtime pack after installing the enemy rig renderer.
inline std::array<SkeletonProfile,10> enemySkeletons{};
inline const EnemySpec& spec(EnemyKind kind) { return specs[static_cast<size_t>(kind)]; }
inline std::array<const char*,10> enemyNames{"drone","ranger","turret","stalker","guardian","scavenger","sniper","bulwark","interceptor","juggernaut"};

std::filesystem::path characterModelFile(const std::filesystem::path&,int id);
std::filesystem::path enemyModelFile(const std::filesystem::path&,int id);
// The model catalog is not part of the packed media archive. Loading is optional;
// absent/invalid files leave the compiled built-in fallback intact.
// Direct-file variants for standalone Open / Save As dialogs. These do not
// modify the current game folder; only explicit export copies files there.
bool loadCharacterModelAt(const std::filesystem::path& file,int id,std::string* error=nullptr);
bool loadEnemyModelAt(const std::filesystem::path& file,int id,std::string* error=nullptr);
bool saveCharacterModelAt(const std::filesystem::path& file,int id,std::string* error=nullptr);
bool saveEnemyModelAt(const std::filesystem::path& file,int id,std::string* error=nullptr);
bool loadCharacterModel(const std::filesystem::path&,int id,std::string* error=nullptr);
bool loadEnemyModel(const std::filesystem::path&,int id,std::string* error=nullptr);
bool saveCharacterModel(const std::filesystem::path&,int id,std::string* error=nullptr);
bool saveEnemyModel(const std::filesystem::path&,int id,std::string* error=nullptr);
bool setCharacterColor(int id,const char* color);
void loadModelCatalog(const std::filesystem::path& directory);
uint32_t modelFingerprint();
}
