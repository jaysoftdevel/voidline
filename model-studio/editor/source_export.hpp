#pragma once
#include <filesystem>
#include <string>

namespace source_export {
// Generate compiled C++ replacements from the validated editor model files.
// The caller decides whether to copy these files into the game source tree.
bool generate(const std::filesystem::path& project,
              const std::filesystem::path& target,
              std::string& error);
}
