#pragma once
#include "content/skeleton_types.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <string>
#include <vector>

namespace vl::runtime_model {
// VLRM v2 adds joint Z offsets. Decode still supports legacy VLRM v1 files.
// Version 2: 20-byte LE header + six records (14 scalar parameters + 3 poses * 14 xyz).
inline constexpr uint32_t version=2, modelCount=6;
inline constexpr size_t recordFloats=14+skeletonPoseCount*skeletonJointCount*3;
inline constexpr size_t legacyRecordFloats=14+skeletonPoseCount*skeletonJointCount*2;
inline constexpr size_t payloadSize=modelCount*recordFloats*sizeof(float);
inline constexpr size_t legacyPayloadSize=modelCount*legacyRecordFloats*sizeof(float);
inline uint32_t checksum(std::span<const uint8_t> data) {
    uint32_t h=2166136261u;
    for(uint8_t b:data){h^=b;h*=16777619u;}
    return h;
}
inline void writeU32(std::vector<uint8_t>& dst,uint32_t n) {
    for(int i=0;i<4;++i)dst.push_back(static_cast<uint8_t>(n>>(8*i)));
}
inline uint32_t readU32(std::span<const uint8_t> bytes,size_t& at) {
    if(at>bytes.size()||bytes.size()-at<4)throw std::runtime_error("Truncated VLRM data");
    uint32_t n=0;for(int i=0;i<4;++i)n|=uint32_t(bytes[at++])<<(8*i);
    return n;
}
inline bool valid(const SkeletonProfile& p) {
    auto range=[](float n,float lo,float hi){return std::isfinite(n)&&n>=lo&&n<=hi;};
    for (auto layer:p.offsets)for(auto point:layer)
        if(!range(point.x,-25.f,25.f)||!range(point.y,-25.f,25.f))return false;
    for (auto layer:p.depths)for(float depth:layer)
        if(!range(depth,-25.f,25.f))return false;
    for(float n:{p.stride,p.armSwing,p.aimRaise,p.bodyHeight,p.bodyWidth,p.headScale,
        p.armLength,p.legLength,p.walkCycle,p.acceleration,p.braking,p.recoil,p.idleBreath})
        if(!range(n,.5f,1.8f))return false;
    return range(p.weaponSeat,-15.f,15.f);
}
inline std::array<float,recordFloats> flatten(const SkeletonProfile& p){
    std::array<float,recordFloats> numbers{};size_t i=0;
    for(float n:{p.stride,p.armSwing,p.aimRaise,p.bodyHeight,p.bodyWidth,p.headScale,
        p.armLength,p.legLength,p.weaponSeat,p.walkCycle,p.acceleration,p.braking,p.recoil,p.idleBreath})
        numbers[i++]=n;
    for(size_t pose=0;pose<skeletonPoseCount;++pose)
        for(size_t joint=0;joint<skeletonJointCount;++joint){
            numbers[i++]=p.offsets[pose][joint].x;
            numbers[i++]=p.offsets[pose][joint].y;
            numbers[i++]=p.depths[pose][joint];
        }
    return numbers;
}
inline SkeletonProfile inflate(const std::array<float,recordFloats>& nums){
    SkeletonProfile p{};size_t i=0;
    p.stride=nums[i++];p.armSwing=nums[i++];p.aimRaise=nums[i++];
    p.bodyHeight=nums[i++];p.bodyWidth=nums[i++];p.headScale=nums[i++];
    p.armLength=nums[i++];p.legLength=nums[i++];p.weaponSeat=nums[i++];
    p.walkCycle=nums[i++];p.acceleration=nums[i++];p.braking=nums[i++];
    p.recoil=nums[i++];p.idleBreath=nums[i++];
    for(size_t pose=0;pose<skeletonPoseCount;++pose)
        for(size_t joint=0;joint<skeletonJointCount;++joint){
            p.offsets[pose][joint].x=nums[i++];
            p.offsets[pose][joint].y=nums[i++];
            p.depths[pose][joint]=nums[i++];
        }
    return p;
}
inline std::vector<uint8_t> encode(const std::array<SkeletonProfile,modelCount>& profiles){
    std::vector<uint8_t> bytes;
    bytes.reserve(20+payloadSize);
    bytes.insert(bytes.end(),{'V','L','R','M'});
    writeU32(bytes,version);writeU32(bytes,modelCount);writeU32(bytes,static_cast<uint32_t>(payloadSize));
    writeU32(bytes,0);
    for(const auto& p:profiles){
        if(!valid(p))throw std::invalid_argument("Invalid skeleton proportions, animation or joint offsets");
        for(float n:flatten(p))writeU32(bytes,std::bit_cast<uint32_t>(n));
    }
    const auto sum=checksum(std::span<const uint8_t>(bytes).subspan(20));
    for(int i=0;i<4;++i)bytes[16+i]=static_cast<uint8_t>(sum>>(8*i));
    return bytes;
}
inline bool decode(std::span<const uint8_t> bytes,std::array<SkeletonProfile,modelCount>& dest,std::string* error=nullptr){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(bytes.size()<20||bytes[0]!='V'||bytes[1]!='L'||bytes[2]!='R'||bytes[3]!='M')
        return fail("Invalid VLRM header");
    size_t at=4;
    const uint32_t fileVersion=readU32(bytes,at);
    const uint32_t count=readU32(bytes,at),payload=readU32(bytes,at);
    const bool legacy=fileVersion==1;
    if((fileVersion!=version&&!legacy)||count!=modelCount||
       payload!=(legacy?legacyPayloadSize:payloadSize)||bytes.size()!=20+payload)
        return fail("Unsupported VLRM version or payload length");
    const auto expected=readU32(bytes,at);
    if(checksum(bytes.subspan(20))!=expected)return fail("VLRM checksum mismatch");
    std::array<SkeletonProfile,modelCount> temporary{};
    for(auto& p:temporary){
        std::array<float,recordFloats> nums{};
        if(legacy){
            for(size_t i=0;i<14;++i)nums[i]=std::bit_cast<float>(readU32(bytes,at));
            size_t j=14;
            for(size_t i=0;i<skeletonPoseCount*skeletonJointCount;++i){
                nums[j++]=std::bit_cast<float>(readU32(bytes,at));
                nums[j++]=std::bit_cast<float>(readU32(bytes,at));
                nums[j++]=0.f;
            }
        }else for(auto& n:nums)n=std::bit_cast<float>(readU32(bytes,at));
        p=inflate(nums);
        if(!valid(p))return fail("VLRM contains invalid or non-finite model values");
    }
    dest=temporary;
    return true;
}
} // namespace vl::runtime_model
