#include "services/audio.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
#include <memory>

extern "C" {
struct SndFile;
struct SoundInfo { int64_t frames; int samplerate,channels,format,sections,seekable; };
#ifdef _WIN32
SndFile* sf_wchar_open(const wchar_t*,int,SoundInfo*);
#else
SndFile* sf_open(const char*,int,SoundInfo*);
#endif
int sf_close(SndFile*);
int64_t sf_readf_float(SndFile*,float*,int64_t);
}

namespace vl {
Audio::Audio(const std::filesystem::path& root)
    :assets(root/"audio") {
    SDL_AudioSpec desired{},result{};
    desired.freq=48000;
    desired.format=std::endian::native==std::endian::little?0x8120:0x9120;
    desired.channels=2; desired.samples=1024; desired.callback=callback; desired.userdata=this;
    device=SDL_OpenAudioDevice(nullptr,0,&desired,&result,1);
    if (!device) { std::cerr<<"Audio device unavailable: "<<SDL_GetError()<<'\n'; return; }
    hz=result.freq;
    for (auto* name:{"arc","carbine","checkpoint","cloth","complete","destroy","empty","enemy","failure","guardian","hit-metal","hurt",
        "jump","land","launch","pickup","pistol","reload","shotgun","step-deck","step-grate","step-rock","switch","upgrade"}) {
        auto& group=variations[name];
        if (auto* sample=load(name)) group.push_back(sample);
        for (int i=2;i<=4;++i) {
            const std::string variant=std::string(name)+"-"+std::to_string(i);
            if (!std::filesystem::exists(assets/(variant+".ogg"))) break;
            if (auto* sample=load(variant)) group.push_back(sample);
        }
    }
    SDL_PauseAudioDevice(device,0);
}

Audio::~Audio() { if (device) SDL_CloseAudioDevice(device); }

Audio::Sample* Audio::load(std::string_view name) {
    std::string key(name);
    if (auto it=samples.find(key);it!=samples.end()) return &it->second;
    const auto file=assets/(key+".ogg");
    SoundInfo info{};
#ifdef _WIN32
    const std::unique_ptr<SndFile,decltype(&sf_close)> in(sf_wchar_open(file.c_str(),0x10,&info),sf_close);
#else
    const std::unique_ptr<SndFile,decltype(&sf_close)> in(sf_open(file.c_str(),0x10,&info),sf_close);
#endif
    if (!in || info.frames<=0 || info.frames>16000000 || info.channels<1 || info.channels>2 || info.samplerate<=0) {
        std::cerr<<"Could not decode "<<file<<'\n'; return nullptr;
    }
    Sample s{{},info.channels,info.samplerate};
    s.frames.resize(size_t(info.frames)*info.channels);
    const auto n=sf_readf_float(in.get(),s.frames.data(),info.frames);
    if (n<=0) return nullptr;
    s.frames.resize(size_t(n)*info.channels);
    return &samples.emplace(key,std::move(s)).first->second;
}

void Audio::configure(const Settings& settings) {
    if (!device) return;
    SDL_LockAudioDevice(device);
    musicGain=settings.audio?settings.music*.70f:0;
    effectsGain=settings.audio?settings.effects*.90f:0;
    SDL_UnlockAudioDevice(device);
}

void Audio::pace(float value) {
    if (!device) return;
    SDL_LockAudioDevice(device);
    speedTarget=clamp(value,.2f,1.f);
    SDL_UnlockAudioDevice(device);
}

void Audio::preload(std::string_view name) {
    if (device) (void)load(name);
}
void Audio::track(std::string_view name,bool loop,float position) {
    if (!device) return;
    const Sample* sample=name.empty()?nullptr:load(name);
    SDL_LockAudioDevice(device);
    if (music.sample!=sample) {
        if (music.sample) previous=music;
        const double frames=sample?double(sample->frames.size()/sample->channels):0;
        const double start=sample?std::clamp(double(position)*sample->hz,0.0,std::max(0.0,frames-1)):0;
        music={sample,start,0,loop};
    }
    SDL_UnlockAudioDevice(device);
}

void Audio::event(std::string_view name,float gain,float pan,float rate) {
    if (!device) return;
    const std::string key(name);
    const auto group=variations.find(key);
    if (group==variations.end() || group->second.empty()) return;
    const auto& choices=group->second;
    const Sample* sample=choices[cursors[key]++%choices.size()];
    const float bearing=clamp(pan,-1,1), distance=std::abs(bearing);
    const float angle=(bearing+1)*pi*.25f;
    const float gainAtDistance=gain/(1+distance*.78f);
    SDL_LockAudioDevice(device);
    auto found=std::find_if(voices.begin(),voices.end(),[](const auto& v){return !v.sample;});
    if (found==voices.end()) found=std::min_element(voices.begin(),voices.end(),[](const auto& a,const auto& b) {
        const auto remaining=[](const Voice& v) {
            const double progress=v.position/(v.sample->frames.size()/v.sample->channels);
            return (v.left+v.right)*(1-progress);
        };
        return remaining(a)<remaining(b);
    });
    *found={sample,0,double(rate)*sample->hz/hz,gainAtDistance*std::cos(angle),gainAtDistance*std::sin(angle),
        1-distance*.55f,1-distance*.86f,0,0};
    SDL_UnlockAudioDevice(device);
}

void Audio::callback(void* self,U8* buffer,int bytes) { static_cast<Audio*>(self)->mix(reinterpret_cast<float*>(buffer),bytes/8); }

void Audio::mix(float* data,int frames) {
    auto sampleAt=[](const Sample& s,double p,int channel) {
        const size_t i=static_cast<size_t>(p);
        const size_t limit=s.frames.size()/s.channels-1;
        const int c=std::min(channel,s.channels-1);
        return vl::mix(s.frames[i*s.channels+c],s.frames[std::min(i+1,limit)*s.channels+c],float(p-i));
    };
    for (int frame=0;frame<frames;++frame) {
        float left=0,right=0;
        speed+=(speedTarget-speed)*.0006f;
        auto musicSample=[&](Track& t,bool fading) {
            if (!t.sample) return;
            const auto& sample=*t.sample;
            const size_t n=sample.frames.size()/sample.channels;
            t.fade=approach(t.fade,fading?0.f:1.f,3.f/hz);
            const float gain=musicGain*t.fade;
            left+=sampleAt(sample,t.position,0)*gain; right+=sampleAt(sample,t.position,1)*gain;
            t.position+=double(sample.hz)/hz*(.55+.45*speed);
            if (fading && t.fade<=0) t.sample=nullptr;
            if (t.position>=n) {
                if (t.loop) t.position=std::fmod(t.position,double(n));
                else t.sample=nullptr;
            }
        };
        musicSample(music,false); musicSample(previous,true);
        for (auto& v:voices) {
            if (!v.sample) continue;
            const float l=sampleAt(*v.sample,v.position,0),r=sampleAt(*v.sample,v.position,1);
            const float mid=(l+r)*.5f, side=(l-r)*.5f*v.stereo;
            v.filteredLeft=vl::mix(v.filteredLeft,mid+side,v.filter);
            v.filteredRight=vl::mix(v.filteredRight,mid-side,v.filter);
            left+=v.filteredLeft*v.left*effectsGain;
            right+=v.filteredRight*v.right*effectsGain;
            v.position+=v.rate*(.62+.38*speed);
            if (v.position>=v.sample->frames.size()/v.sample->channels) v.sample=nullptr;
        }
        const float peak=std::max(std::abs(left),std::abs(right));
        const float available=std::min(1.f,.94f/std::max(.001f,peak));
        limiter=std::min(available,limiter+(1-limiter)*.00048f);
        data[frame*2]=left*limiter;
        data[frame*2+1]=right*limiter;
    }
}
} // namespace vl
