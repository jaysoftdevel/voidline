#include "services.hpp"
#include <bit>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

extern "C" {
struct json_object;
json_object* json_tokener_parse(const char*);
int json_object_get_type(const json_object*);
int json_object_put(json_object*);
json_object* json_object_object_get(const json_object*,const char*);
json_object* json_object_array_get_idx(const json_object*,size_t);
size_t json_object_array_length(const json_object*);
const char* json_object_get_string(json_object*);
int json_object_get_boolean(const json_object*);
int64_t json_object_get_int64(const json_object*);
double json_object_get_double(const json_object*);
json_object* json_object_new_object();
json_object* json_object_new_array();
json_object* json_object_new_string(const char*);
json_object* json_object_new_int64(int64_t);
json_object* json_object_new_double(double);
json_object* json_object_new_boolean(int);
int json_object_object_add(json_object*,const char*,json_object*);
int json_object_array_add(json_object*,json_object*);
const char* json_object_to_json_string_ext(json_object*,int);
struct SndFile;
struct SoundInfo { int64_t frames; int samplerate,channels,format,sections,seekable; };
SndFile* sf_open(const char*,int,SoundInfo*);
int sf_close(SndFile*);
int64_t sf_readf_float(SndFile*,float*,int64_t);
}
namespace vl {
namespace {
using Json=std::unique_ptr<json_object,decltype(&json_object_put)>;
constexpr int jsonBool=1,jsonDouble=2,jsonInt=3,jsonObject=4,jsonArray=5,jsonString=6;
Json parse(const char* data) { return Json(json_tokener_parse(data),json_object_put); }
Json read(const std::filesystem::path& file) {
    std::ifstream in(file);
    const std::string data{std::istreambuf_iterator<char>(in),{}};
    return parse(data.c_str());
}
std::string slotKey(int slot) { return "voidline-save-"+std::to_string(slot); }
constexpr auto settingsKey="voidline-options-v1";
Json record(const std::filesystem::path& file,const std::string& key) {
    auto root=read(file);
    auto* inner=json_object_object_get(root.get(),key.c_str());
    return parse(json_object_get_type(inner)==jsonString?json_object_get_string(inner):"null");
}
int64_t integer(json_object* j,const char* name,int64_t fallback,int64_t lo,int64_t hi) {
    j=json_object_object_get(j,name);
    if (json_object_get_type(j)!=jsonInt) return fallback;
    const int64_t value=json_object_get_int64(j);
    return value<lo || value>hi?fallback:value;
}
float real(json_object* j,const char* name,float fallback) {
    j=json_object_object_get(j,name);
    const int type=json_object_get_type(j);
    if (type!=jsonDouble && type!=jsonInt) return fallback;
    const double value=json_object_get_double(j);
    return std::isfinite(value)?float(value):fallback;
}
bool flag(json_object* j,const char* name,bool fallback) {
    j=json_object_object_get(j,name);
    return json_object_get_type(j)==jsonBool?json_object_get_boolean(j)!=0:fallback;
}
struct File {
    int fd;
    explicit File(int value):fd(value) {}
    ~File() { if (fd>=0) close(fd); }
    File(const File&)=delete;
};
void add(json_object* parent,const char* key,int64_t value) {
    json_object_object_add(parent,key,json_object_new_int64(value));
}
void addReal(json_object* parent,const char* key,float value) {
    json_object_object_add(parent,key,json_object_new_double(value));
}
void addFlag(json_object* parent,const char* key,bool value) {
    json_object_object_add(parent,key,json_object_new_boolean(value));
}
}
std::filesystem::path storagePath() {
    if (const auto* p=std::getenv("XDG_DATA_HOME"); p && p[0]=='/') return std::filesystem::path(p)/"voidline/campaign.json";
    if (const auto* p=std::getenv("HOME"); p && *p) return std::filesystem::path(p)/".local/share/voidline/campaign.json";
    throw std::runtime_error("Set XDG_DATA_HOME or HOME to select a save location.");
}
std::optional<Save> Storage::get(int slot) const {
    auto j=record(file,slotKey(slot));
    if (json_object_get_type(j.get())!=jsonObject) return std::nullopt;
    auto* obj=j.get();
    Save s;
    auto* cleared=json_object_object_get(obj,"cleared");
    if (json_object_get_type(cleared)==jsonArray) {
        for (int candidate=0;candidate<5;++candidate) {
            bool found=false;
            for (size_t i=0;i<json_object_array_length(cleared);++i) {
                auto* item=json_object_array_get_idx(cleared,i);
                if (json_object_get_type(item)==jsonInt && json_object_get_int64(item)==candidate) found=true;
            }
            if (!found) break;
            ++s.cleared;
        }
    }
    constexpr int max=std::numeric_limits<int>::max();
    s.score=integer(obj,"score",0,0,max);
    s.credits=integer(obj,"credits",0,0,max);
    s.character=integer(obj,"character",0,0,2);
    s.difficulty=integer(obj,"difficulty",1,0,2);
    s.updated=integer(obj,"updated",0,0,std::numeric_limits<int64_t>::max());
    const int w=integer(obj,"equipped",0,0,2);
    if (s.cleared>=weapons[w].unlock) s.equipped=w;
    auto* tiers=json_object_object_get(obj,"upgrades");
    auto* records=json_object_object_get(obj,"records");
    for (int i=0;i<3;++i) {
        auto* rank=json_object_get_type(tiers)==jsonArray?json_object_array_get_idx(tiers,i):nullptr;
        for (int j=0;j<4;++j) {
            const auto value=integer(rank,upgrades[j].key,0,std::numeric_limits<int>::min(),max);
            s.tiers[i][j]=std::clamp<int64_t>(value,0,3);
        }
    }
    for (int i=0;i<s.cleared;++i) {
        auto* r=json_object_object_get(records,std::to_string(i).c_str());
        s.records[i]={std::max(0.f,real(r,"time",0)),int(integer(r,"score",0,0,max))};
    }
    return s;
}
Settings Storage::settings() const {
    auto j=record(file,settingsKey);
    Settings s;
    s.audio=flag(j.get(),"audio",s.audio);
    s.motion=flag(j.get(),"motion",s.motion);
    s.cinematic=flag(j.get(),"cinematic",s.cinematic);
    s.high=flag(j.get(),"high",s.high);
    s.music=clamp(real(j.get(),"music",s.music));
    s.effects=clamp(real(j.get(),"effects",s.effects));
    s.difficulty=integer(j.get(),"difficulty",s.difficulty,0,2);
    return s;
}
void Storage::replace(const std::string& key,const std::string& value) {
    std::filesystem::create_directories(file.parent_path());
    const File lock(open((file.string()+".lock").c_str(),O_CREAT|O_CLOEXEC|O_RDWR,0600));
    if (lock.fd<0 || flock(lock.fd,LOCK_EX)!=0) throw std::runtime_error("Could not lock campaign storage.");
    auto outer=read(file);
    if (json_object_get_type(outer.get())!=jsonObject) {
        if (std::filesystem::exists(file)) throw std::runtime_error("Existing campaign.json could not be read.");
        outer.reset(json_object_new_object());
    }
    json_object_object_add(outer.get(),key.c_str(),json_object_new_string(value.c_str()));
    const auto temporary=file.string()+".tmp."+std::to_string(getpid());
    const File output(open(temporary.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600));
    if (output.fd<0) throw std::runtime_error("Could not create campaign storage.");
    try {
        const std::string data=json_object_to_json_string_ext(outer.get(),0);
        size_t done=0;
        while (done<data.size()) {
            const ssize_t bytes=write(output.fd,data.data()+done,data.size()-done);
            if (bytes<0 && errno==EINTR) continue;
            if (bytes<=0) throw std::runtime_error("Could not write campaign storage.");
            done+=bytes;
        }
        if (fsync(output.fd)!=0) throw std::runtime_error("Could not flush campaign storage.");
        std::filesystem::rename(temporary,file);
        const File parent(open(file.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        if (parent.fd>=0) (void)fsync(parent.fd);
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(temporary,error);
        throw;
    }
}
void Storage::put(int slot,Save& s) {
    const Json j(json_object_new_object(),json_object_put);
    auto* obj=j.get();
    const int64_t now=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    add(obj,"updated",now);
    add(obj,"score",s.score); add(obj,"credits",s.credits);
    add(obj,"character",s.character); add(obj,"difficulty",s.difficulty); add(obj,"equipped",s.equipped);
    auto* cleared=json_object_new_array();
    auto* records=json_object_new_object();
    for (int i=0;i<s.cleared;++i) {
        json_object_array_add(cleared,json_object_new_int64(i));
        if (s.records[i].time<=0) continue;
        auto* r=json_object_new_object();
        add(r,"score",s.records[i].score); addReal(r,"time",s.records[i].time);
        json_object_object_add(records,std::to_string(i).c_str(),r);
    }
    auto* tiers=json_object_new_array();
    for (const auto& weapon:s.tiers) {
        auto* u=json_object_new_object();
        for (int i=0;i<4;++i) add(u,upgrades[i].key,weapon[i]);
        json_object_array_add(tiers,u);
    }
    json_object_object_add(obj,"cleared",cleared);
    json_object_object_add(obj,"records",records);
    json_object_object_add(obj,"upgrades",tiers);
    replace(slotKey(slot),json_object_to_json_string_ext(obj,0));
    s.updated=now;
}
void Storage::put(const Settings& s) {
    const Json j(json_object_new_object(),json_object_put);
    auto* obj=j.get();
    addFlag(obj,"audio",s.audio); addFlag(obj,"motion",s.motion);
    addFlag(obj,"cinematic",s.cinematic); addFlag(obj,"high",s.high);
    addReal(obj,"music",s.music); addReal(obj,"effects",s.effects); add(obj,"difficulty",s.difficulty);
    replace(settingsKey,json_object_to_json_string_ext(obj,0));
}

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
    const std::unique_ptr<SndFile,decltype(&sf_close)> in(sf_open(file.c_str(),0x10,&info),sf_close);
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
void Audio::track(std::string_view name,bool loop) {
    if (!device) return;
    const Sample* sample=name.empty()?nullptr:load(name);
    SDL_LockAudioDevice(device);
    if (music.sample!=sample) {
        if (music.sample) previous=music;
        music={sample,0,0,loop};
    }
    SDL_UnlockAudioDevice(device);
}
void Audio::event(std::string_view name,float gain,float pan,float rate) {
    if (!device) return;
    const std::string key(name);
    const auto group=variations.find(key);
    if (group==variations.end() || group->second.empty()) return;
    const auto& samples=group->second;
    const Sample* sample=samples[cursors[key]++%samples.size()];
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
}
