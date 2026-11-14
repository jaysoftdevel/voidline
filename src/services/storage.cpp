#include "services/storage.hpp"
#include "platform/paths.hpp"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

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
#ifdef _WIN32
struct File {
    HANDLE handle;
    explicit File(HANDLE value):handle(value) {}
    ~File() { close(); }
    File(const File&)=delete;
    bool valid() const { return handle!=INVALID_HANDLE_VALUE; }
    void close() {
        if (valid()) CloseHandle(handle);
        handle=INVALID_HANDLE_VALUE;
    }
};
#else
struct File {
    int fd;
    explicit File(int value):fd(value) {}
    ~File() { if (fd>=0) close(fd); }
    File(const File&)=delete;
};
#endif
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
std::filesystem::path storagePath() { return userDataDirectory()/"campaign.json"; }

std::optional<Save> Storage::get(int slot) const {
    auto j=record(file,slotKey(slot));
    if (json_object_get_type(j.get())!=jsonObject) return std::nullopt;
    auto* obj=j.get();
    Save s;
    auto* cleared=json_object_object_get(obj,"cleared");
    if (json_object_get_type(cleared)==jsonArray) {
        for (int candidate=0;candidate<sectorCount;++candidate) {
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
    const int w=integer(obj,"equipped",0,0,int(weaponCount)-1);
    if (s.cleared>=weapons[w].unlock) s.equipped=w;
    auto* tiers=json_object_object_get(obj,"upgrades");
    auto* records=json_object_object_get(obj,"records");
    for (size_t i=0;i<weaponCount;++i) {
        auto* rank=json_object_get_type(tiers)==jsonArray?json_object_array_get_idx(tiers,i):nullptr;
        for (size_t k=0;k<upgradeCount;++k) {
            const auto value=integer(rank,upgrades[k].key,0,std::numeric_limits<int>::min(),max);
            s.tiers[i][k]=int(std::clamp<int64_t>(value,0,3));
        }
    }
    auto* talents=json_object_object_get(obj,"perks");
    for (size_t i=0;i<perkCount;++i) {
        const auto* entry=json_object_get_type(talents)==jsonObject?json_object_object_get(talents,perks[i].key):nullptr;
        s.perks[i]=json_object_get_type(entry)==jsonInt?int(std::clamp<int64_t>(json_object_get_int64(entry),0,perks[i].maxRank)):0;
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
    s.sprintMode=integer(j.get(),"sprintMode",s.sprintMode,0,2);
    s.crouchToggle=flag(j.get(),"crouchToggle",s.crouchToggle);
    s.diagnostics=flag(j.get(),"diagnostics",s.diagnostics);
    auto* keys=json_object_object_get(j.get(),"keymap");
    if (json_object_get_type(keys)==jsonArray && json_object_array_length(keys)==keymapSize) {
        for (size_t i=0;i<keymapSize;++i) {
            auto* item=json_object_array_get_idx(keys,i);
            if (json_object_get_type(item)==jsonInt) s.keymap[i]=int(std::clamp<int64_t>(json_object_get_int64(item),-1,511));
        }
    }
    return s;
}

void Storage::replace(const std::string& key,const std::string& value) {
    const auto directory=file.has_parent_path()?file.parent_path():std::filesystem::current_path();
    std::filesystem::create_directories(directory);
    auto lockPath=file;
    lockPath+=".lock";
#ifdef _WIN32
    const File lock(CreateFileW(lockPath.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                               nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
    OVERLAPPED range{};
    if (!lock.valid() || !LockFileEx(lock.handle,LOCKFILE_EXCLUSIVE_LOCK,0,1,0,&range))
        throw std::runtime_error("Could not lock campaign storage.");
#else
    const File lock(open(lockPath.c_str(),O_CREAT|O_CLOEXEC|O_RDWR,0600));
    if (lock.fd<0 || flock(lock.fd,LOCK_EX)!=0) throw std::runtime_error("Could not lock campaign storage.");
#endif
    auto outer=read(file);
    if (json_object_get_type(outer.get())!=jsonObject) {
        if (std::filesystem::exists(file)) throw std::runtime_error("Existing campaign.json could not be read.");
        outer.reset(json_object_new_object());
    }
    json_object_object_add(outer.get(),key.c_str(),json_object_new_string(value.c_str()));
    const std::string data=json_object_to_json_string_ext(outer.get(),0);
    auto temporary=file;
#ifdef _WIN32
    temporary+=".tmp."+std::to_string(GetCurrentProcessId());
#else
    temporary+=".tmp."+std::to_string(getpid());
#endif
    try {
#ifdef _WIN32
        File output(CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
        if (!output.valid()) throw std::runtime_error("Could not create campaign storage.");
        size_t done=0;
        while (done<data.size()) {
            DWORD written=0;
            const auto chunk=static_cast<DWORD>(std::min<size_t>(data.size()-done,1u<<20));
            if (!WriteFile(output.handle,data.data()+done,chunk,&written,nullptr) || !written)
                throw std::runtime_error("Could not write campaign storage.");
            done+=written;
        }
        if (!FlushFileBuffers(output.handle)) throw std::runtime_error("Could not flush campaign storage.");
        output.close();
        if (!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not replace campaign storage.");
#else
        const File output(open(temporary.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600));
        if (output.fd<0) throw std::runtime_error("Could not create campaign storage.");
        size_t done=0;
        while (done<data.size()) {
            const ssize_t bytes=write(output.fd,data.data()+done,data.size()-done);
            if (bytes<0 && errno==EINTR) continue;
            if (bytes<=0) throw std::runtime_error("Could not write campaign storage.");
            done+=bytes;
        }
        if (fsync(output.fd)!=0) throw std::runtime_error("Could not flush campaign storage.");
        std::filesystem::rename(temporary,file);
        const File parent(open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        if (parent.fd>=0) (void)fsync(parent.fd);
#endif
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
        for (size_t i=0;i<upgradeCount;++i) add(u,upgrades[i].key,weapon[i]);
        json_object_array_add(tiers,u);
    }
    json_object_object_add(obj,"cleared",cleared);
    json_object_object_add(obj,"records",records);
    json_object_object_add(obj,"upgrades",tiers);
    auto* talents=json_object_new_object();
    for (size_t i=0;i<perkCount;++i) add(talents,perks[i].key,s.perks[i]);
    json_object_object_add(obj,"perks",talents);
    replace(slotKey(slot),json_object_to_json_string_ext(obj,0));
    s.updated=now;
}

void Storage::put(const Settings& s) {
    const Json j(json_object_new_object(),json_object_put);
    auto* obj=j.get();
    addFlag(obj,"audio",s.audio); addFlag(obj,"motion",s.motion);
    addFlag(obj,"cinematic",s.cinematic); addFlag(obj,"high",s.high);
    addReal(obj,"music",s.music); addReal(obj,"effects",s.effects); add(obj,"difficulty",s.difficulty);
    add(obj,"sprintMode",s.sprintMode); addFlag(obj,"crouchToggle",s.crouchToggle); addFlag(obj,"diagnostics",s.diagnostics);
    auto* keys=json_object_new_array();
    for (int code:s.keymap) json_object_array_add(keys,json_object_new_int64(code));
    json_object_object_add(obj,"keymap",keys);
    replace(settingsKey,json_object_to_json_string_ext(obj,0));
}
} // namespace vl
