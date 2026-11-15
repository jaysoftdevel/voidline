#pragma once
// Compact enemy motion profiles for future game-side rig integration.
// Separate magic and count from VLRM, retaining identical XYZ record encoding.
#include "content/runtime_model.hpp"
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

namespace vl::enemy_runtime_model {
inline constexpr size_t modelCount=10;
inline constexpr uint32_t version=1;
inline constexpr size_t payloadSize=modelCount*runtime_model::recordFloats*sizeof(float);
inline std::vector<uint8_t> encode(const std::array<SkeletonProfile,modelCount>& profiles) {
    std::vector<uint8_t> bytes{'V','L','E','M'};
    runtime_model::writeU32(bytes,version);
    runtime_model::writeU32(bytes,modelCount);
    runtime_model::writeU32(bytes,static_cast<uint32_t>(payloadSize));
    runtime_model::writeU32(bytes,0);
    for (const auto& p:profiles) {
        if(!runtime_model::valid(p))throw std::invalid_argument("Invalid enemy motion vectors");
        for(float v:runtime_model::flatten(p))runtime_model::writeU32(bytes,std::bit_cast<uint32_t>(v));
    }
    const auto sum=runtime_model::checksum(std::span<const uint8_t>(bytes).subspan(20));
    for(int i=0;i<4;++i)bytes[size_t(16+i)]=uint8_t(sum>>(8*i));
    return bytes;
}
inline bool decode(std::span<const uint8_t> bytes,std::array<SkeletonProfile,modelCount>& output,std::string* error=nullptr){
    const auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(bytes.size()!=20+payloadSize||bytes[0]!='V'||bytes[1]!='L'||bytes[2]!='E'||bytes[3]!='M')
        return fail("Invalid VLEM header/size");
    size_t at=4;
    if(runtime_model::readU32(bytes,at)!=version||runtime_model::readU32(bytes,at)!=modelCount||
       runtime_model::readU32(bytes,at)!=payloadSize)return fail("Unsupported enemy motion format");
    const auto expected=runtime_model::readU32(bytes,at);
    if(runtime_model::checksum(bytes.subspan(20))!=expected)return fail("Enemy motion checksum mismatch");
    std::array<SkeletonProfile,modelCount> parsed{};
    for(auto& p:parsed){
        std::array<float,runtime_model::recordFloats> numbers{};
        for(float& n:numbers)n=std::bit_cast<float>(runtime_model::readU32(bytes,at));
        p=runtime_model::inflate(numbers);
        if(!runtime_model::valid(p))return fail("Invalid enemy motion profile");
    }
    output=parsed;return true;
}
}
