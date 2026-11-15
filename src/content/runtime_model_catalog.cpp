#include "content/runtime_model_catalog.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_runtime_model.hpp"
#include "content/enemy_motion.hpp"
#include "content/skeleton.hpp"
#include "content/asset_pack.hpp"
#include <stdexcept>
#include <iostream>

namespace vl {
// Only at startup, before character selection and multiplayer validation.
// Missing optional model preserves the compiled default rig, but a corrupt
// model is a deployment error and is rejected rather than partially applied.
void loadBakedCharacterModels(const std::filesystem::path& assetRoot) {
    AssetPack pack(assetRoot);
    constexpr const char* file="models/characters.vlrm";
    if (!pack.contains(file)) {std::cout<<"Character models: built-in defaults\n";return;}
    const auto bytes=pack.read(file);
    std::array<SkeletonProfile,6> decoded{};
    std::string error;
    if(!runtime_model::decode(bytes,decoded,&error))
        throw std::runtime_error("Invalid bundled character models: "+error);
    characterSkeletons=decoded;
    std::cout<<"Character models: 6 baked rigs loaded from game.vpak\n";
}
void loadBakedEnemyModels(const std::filesystem::path& assetRoot) {
    AssetPack pack(assetRoot);
    constexpr const char* file="models/enemies.vlem";
    if(!pack.contains(file)){std::cout<<"Enemy models: built-in defaults\n";return;}
    const auto bytes=pack.read(file);
    std::array<SkeletonProfile,10> decoded{};
    std::string error;
    if(!enemy_runtime_model::decode(bytes,decoded,&error))
        throw std::runtime_error("Invalid bundled enemy motion models: "+error);
    enemySkeletons=decoded;
    std::cout<<"Enemy models: 10 baked rigs loaded from game.vpak\n";
}
}
