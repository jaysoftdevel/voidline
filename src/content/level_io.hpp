#pragma once
#include "content/types.hpp"
#include <filesystem>
#include <string>

namespace vl {
// Self-contained editable level snapshots, not part of the packed art archive.
std::filesystem::path levelFile(const std::filesystem::path& directory,int sector,bool cooperative);
bool readLevel(const std::filesystem::path& file,const Stage& base,Stage& result,
               int sector,bool cooperative,std::string* error=nullptr);
bool writeLevel(const std::filesystem::path& file,const Stage& level,int sector,
                bool cooperative,std::string* error=nullptr);
uint32_t levelFingerprint(const Stage& stage);
}
