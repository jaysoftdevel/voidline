#pragma once
#include "content/content.hpp"
#include "content/asset_pack.hpp"
#include "game/entities.hpp"
#include "platform/sdl.hpp"
#include <array>
#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace vl {
class Audio final : public AudioSink {
public:
    explicit Audio(const std::filesystem::path& assets);
    ~Audio() override;
    void configure(const Settings& settings);
    // Playback speed for music and effects. Slow motion lowers both together.
    void pace(float speed);
    void preload(std::string_view name);
    void track(std::string_view name,bool loop=true,float position=0);
    void event(std::string_view name,float gain=1,float pan=0,float rate=1) override;
private:
    struct Sample { std::vector<float> frames; int channels=0,hz=0; };
    struct Voice {
        const Sample* sample=nullptr;
        double position=0,rate=1;
        float left=0,right=0,stereo=1,filter=1,filteredLeft=0,filteredRight=0;
    };
    struct Track { const Sample* sample=nullptr; double position=0; float fade=0; bool loop=true; };
    AssetPack assets;
    std::unordered_map<std::string,std::vector<Sample*>> variations;
    std::unordered_map<std::string,size_t> cursors;
    std::unordered_map<std::string,Sample> samples;
    std::array<Voice,32> voices{};
    Track music,previous;
    U32 device=0;
    int hz=48000;
    float musicGain=.74f,effectsGain=.68f,limiter=1,speed=1,speedTarget=1;
    Sample* load(std::string_view name);
    static void callback(void* self,U8* buffer,int length);
    void mix(float* data,int frames);
};
}
