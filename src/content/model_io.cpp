#include "content/model_io.hpp"
#include "content/skeleton.hpp"
#include "content/enemy_motion.hpp"
#include <json-c/json.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

namespace vl {
namespace {
using Json=std::unique_ptr<json_object,decltype(&json_object_put)>;
std::array<std::array<char,8>,6> customColors{};
// Pointers in Character remain valid for the entire lifetime of the process.
std::array<std::array<std::string,6>,6> customTexts{};
std::array<std::string,10> customEnemyNames{};

bool fail(std::string* why,const char* message) {if(why) *why=message;return false;}
json_object* get(json_object* o,const char* field) {
    json_object* value=nullptr;
    return o && json_object_is_type(o,json_type_object)&&json_object_object_get_ex(o,field,&value)?value:nullptr;
}
bool real(json_object* o,const char* key,float& result,float low,float high) {
    auto* v=get(o,key);
    if (!v || !(json_object_is_type(v,json_type_int)||json_object_is_type(v,json_type_double)))return false;
    const double n=json_object_get_double(v);
    if(!std::isfinite(n)||n<low||n>high)return false;
    result=static_cast<float>(n);return true;
}
bool integer(json_object* o,const char* key,int& result,int low,int high) {
    auto* v=get(o,key);
    if(!v||!json_object_is_type(v,json_type_int))return false;
    auto n=json_object_get_int64(v);
    if(n<low||n>high)return false;
    result=static_cast<int>(n);return true;
}
void put(json_object* o,const char* k,float v){json_object_object_add(o,k,json_object_new_double(v));}
void put(json_object* o,const char* k,int v){json_object_object_add(o,k,json_object_new_int(v));}
void put(json_object* o,const char* k,const char* v){json_object_object_add(o,k,json_object_new_string(v));}
Json parse(const std::filesystem::path& file){
    std::ifstream stream(file,std::ios::binary);
    if(!stream)return {nullptr,json_object_put};
    std::string s((std::istreambuf_iterator<char>(stream)),{});
    if(s.size()>65536)return {nullptr,json_object_put};
    return {json_tokener_parse(s.c_str()),json_object_put};
}
bool schema(json_object* j,const char* type,int id) {
    auto* v=get(j,"schema"); int given=-1;
    return v && json_object_is_type(v,json_type_string) &&
       std::strcmp(json_object_get_string(v),type)==0 && integer(j,"id",given,0,15) && id==given;
}
bool write(const std::filesystem::path& path,json_object* object,std::string* error) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(),ec);
    if(ec)return fail(error,"Could not create model directory.");
    const auto tmp=std::filesystem::path(path.string()+".tmp");
    {
        std::ofstream stream(tmp,std::ios::binary|std::ios::trunc);
        if(!stream||!(stream<<json_object_to_json_string_ext(object,JSON_C_TO_STRING_PRETTY)<<'\n'))
            return fail(error,"Could not write model JSON.");
    }
    std::filesystem::rename(tmp,path,ec);
    if(ec){ec.clear();std::filesystem::remove(path,ec);ec.clear();std::filesystem::rename(tmp,path,ec);}
    return ec?fail(error,"Could not replace model JSON."):true;
}
bool isColor(const char* value) {
    if(!value||std::strlen(value)!=7||*value!='#')return false;
    for(int i=1;i<7;++i)if(!std::strchr("0123456789abcdefABCDEF",value[i]))return false;
    return true;
}
// Validate the complete 3-pose rig before mutating the live character catalog.
bool readSkeleton(json_object* parent,SkeletonProfile& result){
    auto* rig=get(parent,"skeleton");
    if(!rig)return false;
    for(size_t pose=0;pose<skeletonPoseCount;++pose){
        auto* points=get(rig,skeletonPoseNames[pose]);
        if(!points||!json_object_is_type(points,json_type_array)||json_object_array_length(points)!=skeletonJointCount)return false;
        for(size_t i=0;i<skeletonJointCount;++i){
            auto* pair=json_object_array_get_idx(points,i);
            if(!pair||!json_object_is_type(pair,json_type_array)||json_object_array_length(pair)!=2)return false;
            float x=0,y=0;
            auto* xObj=json_object_array_get_idx(pair,0),*yObj=json_object_array_get_idx(pair,1);
            if(!xObj||!yObj||!(json_object_is_type(xObj,json_type_double)||json_object_is_type(xObj,json_type_int))||
               !(json_object_is_type(yObj,json_type_double)||json_object_is_type(yObj,json_type_int)))return false;
            const double xd=json_object_get_double(xObj),yd=json_object_get_double(yObj);
            if(!std::isfinite(xd)||!std::isfinite(yd)||xd < -65536||xd > 65536||yd < -65536||yd > 65536)return false;
            x=static_cast<float>(xd);y=static_cast<float>(yd);
            result.offsets[pose][i]={x,y};
        }
    }
    // Optional for v1-v3 compatibility; v4 writes these values explicitly.
    if(auto* depth=get(rig,"depth")){
        for(size_t pose=0;pose<skeletonPoseCount;++pose){
            auto* items=get(depth,skeletonPoseNames[pose]);
            if(!items||!json_object_is_type(items,json_type_array)||json_object_array_length(items)!=skeletonJointCount)return false;
            for(size_t joint=0;joint<skeletonJointCount;++joint){
                auto* value=json_object_array_get_idx(items,joint);
                if(!value || !(json_object_is_type(value,json_type_double)||json_object_is_type(value,json_type_int)))return false;
                const double number=json_object_get_double(value);
                if(!std::isfinite(number)||number< -65536||number>65536)return false;
                result.depths[pose][joint]=static_cast<float>(number);
            }
        }
    }
    return real(rig,"stride",result.stride,.5f,1.8f)&&
           real(rig,"armSwing",result.armSwing,.5f,1.8f)&&
           real(rig,"aimRaise",result.aimRaise,.5f,1.8f);
}
bool readDetails(json_object* parent,SkeletonProfile& p){
    auto* geo=get(parent,"geometry"),*motion=get(parent,"motion");
    return geo && motion &&
        real(geo,"bodyHeight",p.bodyHeight,.5f,1.8f) &&
        real(geo,"bodyWidth",p.bodyWidth,.5f,1.8f) &&
        real(geo,"headScale",p.headScale,.5f,1.8f) &&
        real(geo,"armLength",p.armLength,.5f,1.8f) &&
        real(geo,"legLength",p.legLength,.5f,1.8f) &&
        real(geo,"weaponSeat",p.weaponSeat,-15.f,15.f) &&
        real(motion,"walkCycle",p.walkCycle,.5f,1.8f) &&
        real(motion,"acceleration",p.acceleration,.5f,1.8f) &&
        real(motion,"braking",p.braking,.5f,1.8f) &&
        real(motion,"recoil",p.recoil,.5f,1.8f) &&
        real(motion,"idleBreath",p.idleBreath,.5f,1.8f);
}
void writeSkeleton(json_object* parent,const SkeletonProfile& rig){
    auto* object=json_object_new_object();
    for(size_t pose=0;pose<skeletonPoseCount;++pose){
        auto* points=json_object_new_array();
        for(const auto& offset:rig.offsets[pose]){
            auto* pair=json_object_new_array();
            json_object_array_add(pair,json_object_new_double(offset.x));
            json_object_array_add(pair,json_object_new_double(offset.y));
            json_object_array_add(points,pair);
        }
        json_object_object_add(object,skeletonPoseNames[pose],points);
    }
    auto* depth=json_object_new_object();
    for(size_t pose=0;pose<skeletonPoseCount;++pose){
        auto* points=json_object_new_array();
        for(float d:rig.depths[pose])json_object_array_add(points,json_object_new_double(d));
        json_object_object_add(depth,skeletonPoseNames[pose],points);
    }
    json_object_object_add(object,"depth",depth);
    put(object,"stride",rig.stride);put(object,"armSwing",rig.armSwing);put(object,"aimRaise",rig.aimRaise);
    json_object_object_add(parent,"skeleton",object);
    auto* geometry=json_object_new_object();
    put(geometry,"bodyHeight",rig.bodyHeight);put(geometry,"bodyWidth",rig.bodyWidth);
    put(geometry,"headScale",rig.headScale);put(geometry,"armLength",rig.armLength);
    put(geometry,"legLength",rig.legLength);put(geometry,"weaponSeat",rig.weaponSeat);
    json_object_object_add(parent,"geometry",geometry);
    auto* motion=json_object_new_object();
    put(motion,"walkCycle",rig.walkCycle);put(motion,"acceleration",rig.acceleration);
    put(motion,"braking",rig.braking);put(motion,"recoil",rig.recoil);
    put(motion,"idleBreath",rig.idleBreath);
    json_object_object_add(parent,"motion",motion);
}
}
std::filesystem::path characterModelFile(const std::filesystem::path& root,int id) {
    return root/"characters"/(std::to_string(id)+".vlcharacter.json");
}
std::filesystem::path enemyModelFile(const std::filesystem::path& root,int id) {
    return root/"enemies"/(std::to_string(id)+".vlenemy.json");
}
bool loadCharacterModelAt(const std::filesystem::path& file,int id,std::string* error) {
    if(id<0||id>=int(characters.size()))return fail(error,"Invalid character ID.");
    auto j=parse(file);
    const bool v4=j&&schema(j.get(),"voidline-character-v4",id);
    const bool v3=v4||(j&&schema(j.get(),"voidline-character-v3",id));
    const bool modern=v3||(j&&schema(j.get(),"voidline-character-v2",id));
    if(!modern && (!j||!schema(j.get(),"voidline-character-v1",id)))
        return fail(error,"Invalid character model schema or ID.");
    auto c=characters[static_cast<size_t>(id)];
    if(!integer(j.get(),"health",c.health,25,400)||
        !real(j.get(),"speed",c.speed,100,650)||
        !real(j.get(),"jump",c.jump,250,1000)||
        !real(j.get(),"ammo",c.ammo,.2f,4.f)||
        !integer(j.get(),"rig",c.rig,-1,5))return fail(error,"Invalid character movement or rig parameters.");
    auto* color=get(j.get(),"color");
    if(!color||!json_object_is_type(color,json_type_string)||!isColor(json_object_get_string(color)))
        return fail(error,"Character color must be #RRGGBB.");
    SkeletonProfile profile{};
    if(modern && !readSkeleton(j.get(),profile))return fail(error,"Invalid skeleton vectors or motion multipliers.");
    if(v4 && !get(get(j.get(),"skeleton"),"depth"))return fail(error,"Missing v4 skeleton depth vectors.");
    if(v3 && !readDetails(j.get(),profile))return fail(error,"Invalid body dimensions or movement timing.");
    // Editable identity/description strings are stored as owned text, not
    // pointers into transient json-c buffers. Missing legacy fields retain defaults.
    constexpr const char* keys[]{"name","role","art","trait","description"};
    constexpr size_t maxLengths[]{96,96,96,256,1024};
    std::array<std::string,5> newStrings{};
    std::array<bool,5> present{};
    for(size_t k=0;k<5;++k){
        if(auto* value=get(j.get(),keys[k])){
            if(!json_object_is_type(value,json_type_string))return fail(error,"Invalid character text property.");
            const char* text=json_object_get_string(value);
            const auto size=std::strlen(text);
            if(size>maxLengths[k] || (k==0 && size==0))return fail(error,"Invalid character text length.");
            newStrings[k]=text;present[k]=true;
        }
    }
    if(!setCharacterColor(id,json_object_get_string(color)))return false;
    // setCharacterColor changes the pointer in the catalog, not in the temporary copy.
    c.color=characters[static_cast<size_t>(id)].color;
    auto& slots=customTexts[static_cast<size_t>(id)];
    for(size_t k=0;k<5;++k)if(present[k])slots[k]=std::move(newStrings[k]);
    if(present[0])c.name=slots[0].c_str();
    if(present[1])c.role=slots[1].c_str();
    if(present[2])c.art=slots[2].c_str();
    if(present[3])c.trait=slots[3].c_str();
    if(present[4])c.description=slots[4].c_str();
    characters[static_cast<size_t>(id)]=c;
    characterSkeletons[static_cast<size_t>(id)]=profile;
    return true;
}
bool loadEnemyModelAt(const std::filesystem::path& file,int id,std::string* error) {
    if(id<0||id>=int(specs.size()))return fail(error,"Invalid enemy ID.");
    auto j=parse(file);
    if(!j||!schema(j.get(),"voidline-enemy-v1",id))return fail(error,"Invalid enemy model schema or ID.");
    auto s=specs[static_cast<size_t>(id)];
    if(!real(j.get(),"width",s.w,18,180)||!real(j.get(),"height",s.h,18,180)||
       !real(j.get(),"speed",s.speed,0,450)||!real(j.get(),"reach",s.reach,40,1200)||
       !real(j.get(),"attackDelay",s.delay,0,9)||
       !integer(j.get(),"health",s.hp,1,2000)||!integer(j.get(),"damage",s.damage,0,120)||
       !integer(j.get(),"score",s.score,0,1000))return fail(error,"Invalid enemy size or combat parameters.");
    specs[static_cast<size_t>(id)]=s;return true;
}
bool saveCharacterModelAt(const std::filesystem::path& file,int id,std::string* error) {
    if(id<0||id>=int(characters.size()))return fail(error,"Invalid character ID.");
    const auto& c=characters[static_cast<size_t>(id)];Json j(json_object_new_object(),json_object_put);
    put(j.get(),"schema","voidline-character-v4");put(j.get(),"id",id);
    put(j.get(),"name",c.name);put(j.get(),"role",c.role);
    put(j.get(),"art",c.art);put(j.get(),"trait",c.trait);put(j.get(),"description",c.description);
    put(j.get(),"color",c.color);put(j.get(),"rig",c.rig);
    put(j.get(),"health",c.health);put(j.get(),"speed",c.speed);
    put(j.get(),"jump",c.jump);put(j.get(),"ammo",c.ammo);
    writeSkeleton(j.get(),characterSkeletons[static_cast<size_t>(id)]);
    return write(file,j.get(),error);
}
bool saveEnemyModelAt(const std::filesystem::path& file,int id,std::string* error) {
    if(id<0||id>=int(specs.size()))return fail(error,"Invalid enemy ID.");
    const auto& s=specs[static_cast<size_t>(id)];Json j(json_object_new_object(),json_object_put);
    put(j.get(),"schema","voidline-enemy-v1");put(j.get(),"id",id);
    put(j.get(),"name",enemyNames[id]);put(j.get(),"width",s.w);put(j.get(),"height",s.h);
    put(j.get(),"speed",s.speed);put(j.get(),"reach",s.reach);put(j.get(),"attackDelay",s.delay);
    put(j.get(),"health",s.hp);put(j.get(),"damage",s.damage);put(j.get(),"score",s.score);
    return write(file,j.get(),error);
}
bool loadCharacterModel(const std::filesystem::path& root,int id,std::string* error) {
    return loadCharacterModelAt(characterModelFile(root,id),id,error);
}
bool loadEnemyModel(const std::filesystem::path& root,int id,std::string* error) {
    return loadEnemyModelAt(enemyModelFile(root,id),id,error);
}
bool saveCharacterModel(const std::filesystem::path& root,int id,std::string* error) {
    return saveCharacterModelAt(characterModelFile(root,id),id,error);
}
bool saveEnemyModel(const std::filesystem::path& root,int id,std::string* error) {
    return saveEnemyModelAt(enemyModelFile(root,id),id,error);
}
bool setCharacterColor(int id,const char* color) {
    if(id<0||id>=int(characters.size())||!isColor(color))return false;
    auto& slot=customColors[static_cast<size_t>(id)];
    std::memcpy(slot.data(),color,8);
    characters[static_cast<size_t>(id)].color=slot.data();return true;
}
void loadModelCatalog(const std::filesystem::path& root) {
    for(int i=0;i<int(characters.size());++i)if(std::filesystem::exists(characterModelFile(root,i))){
        std::string err;if(!loadCharacterModel(root,i,&err))std::cerr<<"VOIDLINE character model "<<i<<": "<<err<<'\n';
    }
    for(int i=0;i<int(specs.size());++i)if(std::filesystem::exists(enemyModelFile(root,i))){
        std::string err;if(!loadEnemyModel(root,i,&err))std::cerr<<"VOIDLINE enemy model "<<i<<": "<<err<<'\n';
    }
}
uint32_t modelFingerprint() {
    uint32_t h=2166136261u;
    auto push=[&](uint32_t n){for(int i=0;i<4;++i){h^=(n>>(i*8))&255u;h*=16777619u;}};
    auto f=[&](float n){push(std::bit_cast<uint32_t>(n));};
    for(size_t i=0;i<characters.size();++i){const auto& c=characters[i];
        push(uint32_t(c.health));f(c.speed);f(c.jump);f(c.ammo);push(uint32_t(c.rig));
        for(const char* value:{c.name,c.role,c.art,c.color,c.trait,c.description})
            for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p)push(uint32_t(*p));
        for(const auto& layer:characterSkeletons[i].offsets)for(const Vec joint:layer){f(joint.x);f(joint.y);}
        for(const auto& layer:characterSkeletons[i].depths)for(float z:layer)f(z);
        for(const auto& layer:characterSkeletons[i].actionOffsets)for(const Vec joint:layer){f(joint.x);f(joint.y);}
        for(float n:{characterSkeletons[i].stride,characterSkeletons[i].armSwing,characterSkeletons[i].aimRaise,
            characterSkeletons[i].bodyHeight,characterSkeletons[i].bodyWidth,characterSkeletons[i].headScale,
            characterSkeletons[i].armLength,characterSkeletons[i].legLength,characterSkeletons[i].weaponSeat,
            characterSkeletons[i].walkCycle,characterSkeletons[i].acceleration,characterSkeletons[i].braking,
            characterSkeletons[i].recoil,characterSkeletons[i].idleBreath})f(n);
    }
    for(size_t i=0;i<specs.size();++i){
        const auto& e=specs[i];
        for(const unsigned char* p=reinterpret_cast<const unsigned char*>(enemyNames[i]);*p;++p)push(*p);
        f(e.w);f(e.h);f(e.speed);f(e.reach);f(e.delay);
        push(uint32_t(e.hp));push(uint32_t(e.damage));push(uint32_t(e.score));
    }
    for(const auto& profile:enemySkeletons){
        for(const auto& pose:profile.offsets)for(Vec p:pose){f(p.x);f(p.y);}
        for(const auto& pose:profile.depths)for(float z:pose)f(z);
        for(const auto& pose:profile.actionOffsets)for(const Vec joint:pose){f(joint.x);f(joint.y);}
        for(float n:{profile.stride,profile.armSwing,profile.aimRaise,profile.bodyHeight,profile.bodyWidth,
            profile.headScale,profile.armLength,profile.legLength,profile.weaponSeat,profile.walkCycle,
            profile.acceleration,profile.braking,profile.recoil,profile.idleBreath})f(n);
    }
    return h;
}
} // namespace vl
