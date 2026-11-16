#pragma once
// Colors and small formatting helpers shared by the interface sources.
#include "ui/app.hpp"
#include <algorithm>
#include <limits>
#include <string>

namespace vl::ui {
inline const Color amber=hex(0xffac71), white=hex(0xf2f5fa), pale=hex(0xbbc8d4), muted=hex(0x8397a9), teal=hex(0x8ef4da);
inline std::string digits(int n) { return n<10?"0"+std::to_string(n):std::to_string(n); }
inline std::string timeText(float secs) {
    const int total=std::max(0,int(secs));
    return std::to_string(total/60)+":"+digits(total%60);
}
inline std::string sectorTrack(int index) {
    const auto stem="sector-"+std::to_string(static_cast<int>(stages[index].biome)+1);
    // Three original mixes per biome. The first chapter keeps its familiar
    // opening theme; the later chapters alternate faster combat variations.
    const int group=(index/5)%3;
    return stem+(group==1?"-assault":group==2?"-pursuit":"");
}
inline int total(int current,int earned) { return current+std::min(earned,std::numeric_limits<int>::max()-current); }
inline constexpr Rect screen{0,0,width,height};
}
