#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vl {
// Release media are read from a single opaque runtime package; no extraction on disk.
// This is packaging/obfuscation, not copy protection: a determined user can unpack it.
class AssetPack {
public:
    explicit AssetPack(const std::filesystem::path& root);
    bool contains(std::string_view name) const;
    std::vector<uint8_t> read(std::string_view name) const;
private:
    struct Entry { size_t position,length; uint32_t checksum; };
    std::vector<uint8_t> archive;
    std::unordered_map<std::string,Entry> index;
};
}
