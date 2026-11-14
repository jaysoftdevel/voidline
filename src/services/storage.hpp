#pragma once
#include "content/content.hpp"
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace vl {
class Storage {
public:
    explicit Storage(std::filesystem::path location):file(std::move(location)) {}
    std::optional<Save> get(int slot) const;
    Settings settings() const;
    void put(int slot,Save& save);
    void put(const Settings& settings);
private:
    std::filesystem::path file;
    void replace(const std::string& key,const std::string& value);
};
std::filesystem::path storagePath();
}
