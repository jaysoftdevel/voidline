#include "engine/graphics.hpp"
#include <algorithm>
#include <cctype>

namespace vl {
const char* graphicsName(GraphicsMode mode) {
    switch (mode) {
    case GraphicsMode::automatic: return "Auto";
    case GraphicsMode::opengl: return "OpenGL";
    case GraphicsMode::dx12: return "Direct3D 12";
    case GraphicsMode::warp: return "Direct3D 12 WARP";
    }
    return "Unknown";
}

namespace {
std::string lower(std::string_view text) {
    std::string result(text);
    for (char& c:result) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}
}

bool isDirectXRenderer(std::string_view renderer) {
    const auto name=lower(renderer);
    return name.find("d3d12")!=std::string::npos || name.find("direct3d 12")!=std::string::npos;
}
bool isWarpRenderer(std::string_view renderer) {
    const auto name=lower(renderer);
    return name.find("microsoft basic render driver")!=std::string::npos || name.find("warp")!=std::string::npos;
}
}
