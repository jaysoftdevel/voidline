#pragma once
#include <filesystem>
namespace vl {
void loadBakedCharacterModels(const std::filesystem::path& assetsRoot);
void loadBakedEnemyModels(const std::filesystem::path& assetsRoot);
}
