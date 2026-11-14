#pragma once
namespace vl {
// The version comes from project(VERSION ...) in CMakeLists.txt, the only place it is written down.
#ifdef VOIDLINE_VERSION
inline constexpr const char* version=VOIDLINE_VERSION;
#else
inline constexpr const char* version="dev";
#endif
}
