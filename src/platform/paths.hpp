#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace vl {
std::filesystem::path executableDirectory();
std::filesystem::path userDataDirectory();
std::filesystem::path pathFromUtf8(std::string_view path);
std::string utf8Path(const std::filesystem::path& path);
}
