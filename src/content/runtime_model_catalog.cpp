#include "content/runtime_model_catalog.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_runtime_model.hpp"
#include "content/enemy_motion.hpp"
#include "content/skeleton.hpp"
#include "content/asset_pack.hpp"
#include "content/characters.hpp"
#include "content/model_io.hpp"
#include <stdexcept>
#include <iostream>
#include <json-c/json.h>
#include <array>
#include <string>
#include <cstring>
#include <memory>
#include <cmath>
#include <cctype>

namespace vl {
namespace {
using Json=std::unique_ptr<json_object,decltype(&json_object_put)>;
// Permanent backing storage for the const char* strings in Character.
std::array<std::array<std::string,6>,6> publishedText{};
std::array<std::string,10> publishedEnemyNames{};
template<size_t N>
void loadActionSequences(const AssetPack& pack,const char* name,
                         std::array<SkeletonProfile,N>& profiles) {
    if(!pack.contains(name))return;
    const auto bytes=pack.read(name);
    if(bytes.size()>1024*1024)throw std::runtime_error("Action sequence file too large");
    Json root(json_tokener_parse(std::string(bytes.begin(),bytes.end()).c_str()),json_object_put);
    json_object* models=nullptr;
    if(!root || !json_object_is_type(root.get(),json_type_object) ||
       !json_object_object_get_ex(root.get(),"models",&models) ||
       !json_object_is_type(models,json_type_array) || json_object_array_length(models)!=N)
        throw std::runtime_error("Invalid action sequence catalog");
    for(size_t i=0;i<N;++i){
        json_object* entry=json_object_array_get_idx(models,i),*id=nullptr,*sequences=nullptr;
        if(!entry || !json_object_is_type(entry,json_type_object) ||
           !json_object_object_get_ex(entry,"id",&id) || json_object_get_int(id)!=int(i) ||
           !json_object_object_get_ex(entry,"sequences",&sequences) ||
           !json_object_is_type(sequences,json_type_object))
            throw std::runtime_error("Invalid action profile identity");
        for(size_t action=0;action<actionPoseCount;++action){
            json_object* joints=nullptr;
            if(!json_object_object_get_ex(sequences,actionPoseNames[action],&joints))continue;
            if(!json_object_is_type(joints,json_type_array)||json_object_array_length(joints)!=skeletonJointCount)
                throw std::runtime_error("Incorrect number of action joints");
            for(size_t j=0;j<skeletonJointCount;++j){
                auto* pair=json_object_array_get_idx(joints,j);
                if(!pair||!json_object_is_type(pair,json_type_array)||json_object_array_length(pair)!=2)
                    throw std::runtime_error("Invalid action joint vector");
                float xyz[2]{};
                for(int axis=0;axis<2;++axis){
                    auto* n=json_object_array_get_idx(pair,axis);
                    if(!n || !(json_object_is_type(n,json_type_int)||json_object_is_type(n,json_type_double)))
                        throw std::runtime_error("Invalid joint component");
                    const double value=json_object_get_double(n);
                    if(!std::isfinite(value)||std::abs(value)>65536.0)
                        throw std::runtime_error("Action joint outside allowed range");
                    xyz[axis]=float(value);
                }
                profiles[i].actionOffsets[action][j]={xyz[0],xyz[1]};
            }
        }
    }
    std::cout<<"Action sequences: "<<N<<" profiles loaded from "<<name<<"\n";
}
void loadCharacterMetadata(const AssetPack& pack){
    constexpr const char* file="models/characters.info.json";
    if(!pack.contains(file))return;
    const auto data=pack.read(file);
    if(data.empty()||data.size()>131072)throw std::runtime_error("Invalid character metadata size");
    json_tokener* tok=json_tokener_new();
    if(!tok)throw std::runtime_error("Unable to allocate JSON parser");
    json_object* parsed=json_tokener_parse_ex(tok,reinterpret_cast<const char*>(data.data()),static_cast<int>(data.size()));
    const auto status=json_tokener_get_error(tok);
    json_tokener_free(tok);
    Json root(parsed,json_object_put);
    if(status!=json_tokener_success || !root || !json_object_is_type(root.get(),json_type_array) ||
       json_object_array_length(root.get())!=characters.size())
        throw std::runtime_error("Invalid published character metadata");
    // Do not commit partial edits if one model record is invalid.
    auto incoming=characters;
    std::array<std::array<std::string,6>,6> nextText{};
    constexpr const char* keys[]{"name","role","art","color","trait","description"};
    const size_t limits[]{96,96,96,7,256,1024};
    for(size_t id=0;id<incoming.size();++id){
        auto* entry=json_object_array_get_idx(root.get(),static_cast<int>(id));
        json_object* value=nullptr;
        if(!entry||!json_object_is_type(entry,json_type_object)||
           !json_object_object_get_ex(entry,"id",&value)||json_object_get_int(value)!=static_cast<int>(id))
            throw std::runtime_error("Character metadata ID mismatch");
        for(size_t k=0;k<6;++k){
            if(!json_object_object_get_ex(entry,keys[k],&value))continue;
            if(!json_object_is_type(value,json_type_string))throw std::runtime_error("Invalid metadata text");
            const char* text=json_object_get_string(value);
            const size_t n=std::strlen(text);
            if(n>limits[k]||(k==0&&n==0))throw std::runtime_error("Invalid metadata text length");
            if(k==3){
                if(n!=7||text[0]!='#')throw std::runtime_error("Invalid model color");
                for(size_t j=1;j<n;++j)if(!std::isxdigit(static_cast<unsigned char>(text[j])))
                    throw std::runtime_error("Invalid color channel");
            }
            nextText[id][k]=text;
        }
        auto number=[&](const char* key,double lo,double hi,double& out){
            if(!json_object_object_get_ex(entry,key,&value) ||
               !(json_object_is_type(value,json_type_double)||json_object_is_type(value,json_type_int)))
                return false;
            out=json_object_get_double(value);
            return std::isfinite(out)&&out>=lo&&out<=hi;
        };
        double v=0;
        if(!number("health",25,400,v))throw std::runtime_error("Invalid model health");
        incoming[id].health=static_cast<int>(v);
        if(!number("speed",100,650,v))throw std::runtime_error("Invalid model speed");
        incoming[id].speed=static_cast<float>(v);
        if(!number("jump",250,1000,v))throw std::runtime_error("Invalid model jump");
        incoming[id].jump=static_cast<float>(v);
        if(!number("ammo",.2,4,v))throw std::runtime_error("Invalid model ammo");
        incoming[id].ammo=static_cast<float>(v);
        if(!number("rig",-1,5,v))throw std::runtime_error("Invalid model rig");
        incoming[id].rig=static_cast<int>(v);
    }
    publishedText=std::move(nextText);
    for(size_t id=0;id<incoming.size();++id){
        auto& c=incoming[id];const auto& t=publishedText[id];
        if(!t[0].empty())c.name=t[0].c_str();
        if(!t[1].empty())c.role=t[1].c_str();
        if(!t[2].empty())c.art=t[2].c_str();
        if(!t[3].empty())c.color=t[3].c_str();
        if(!t[4].empty())c.trait=t[4].c_str();
        if(!t[5].empty())c.description=t[5].c_str();
    }
    characters=incoming;
    std::cout<<"Character metadata: 6 profiles loaded from game.vpak\n";
}
void loadEnemyMetadata(const AssetPack& pack){
    constexpr const char* file="models/enemies.info.json";
    if(!pack.contains(file))return;
    const auto data=pack.read(file);
    if(data.empty()||data.size()>131072)throw std::runtime_error("Invalid enemy metadata size");
    json_tokener* tok=json_tokener_new();
    if(!tok)throw std::runtime_error("Unable to allocate JSON parser");
    json_object* parsed=json_tokener_parse_ex(tok,reinterpret_cast<const char*>(data.data()),static_cast<int>(data.size()));
    const auto status=json_tokener_get_error(tok);json_tokener_free(tok);
    Json root(parsed,json_object_put);
    if(status!=json_tokener_success||!root||!json_object_is_type(root.get(),json_type_array)||
       json_object_array_length(root.get())!=specs.size())
       throw std::runtime_error("Invalid enemy metadata catalog");
    auto incoming=specs;
    std::array<std::string,10> names{};
    for(size_t id=0;id<incoming.size();++id){
        auto* entry=json_object_array_get_idx(root.get(),static_cast<int>(id));
        json_object* value=nullptr;
        if(!entry||!json_object_is_type(entry,json_type_object)||
           !json_object_object_get_ex(entry,"id",&value)||json_object_get_int(value)!=static_cast<int>(id))
           throw std::runtime_error("Enemy metadata ID mismatch");
        if(!json_object_object_get_ex(entry,"name",&value)||!json_object_is_type(value,json_type_string))
           throw std::runtime_error("Invalid enemy name");
        names[id]=json_object_get_string(value);
        if(names[id].empty()||names[id].size()>96)throw std::runtime_error("Invalid enemy name length");
        auto read=[&](const char* key,double lo,double hi)->double{
            if(!json_object_object_get_ex(entry,key,&value)||
               !(json_object_is_type(value,json_type_int)||json_object_is_type(value,json_type_double)))
                throw std::runtime_error(std::string("Invalid enemy parameter: ")+key);
            const double n=json_object_get_double(value);
            if(!std::isfinite(n)||n<lo||n>hi)throw std::runtime_error(std::string("Enemy value out of range: ")+key);
            return n;
        };
        auto& e=incoming[id];
        e.w=static_cast<float>(read("width",18,180));
        e.h=static_cast<float>(read("height",18,180));
        e.speed=static_cast<float>(read("speed",0,450));
        e.reach=static_cast<float>(read("reach",40,1200));
        e.delay=static_cast<float>(read("attackDelay",0,9));
        e.hp=static_cast<int>(read("health",1,2000));
        e.damage=static_cast<int>(read("damage",0,120));
        e.score=static_cast<int>(read("score",0,1000));
    }
    publishedEnemyNames=std::move(names);
    for(size_t id=0;id<publishedEnemyNames.size();++id)enemyNames[id]=publishedEnemyNames[id].c_str();
    specs=incoming;
    std::cout<<"Enemy metadata: 10 profiles loaded from game.vpak\n";
}
}
// Only at startup, before character selection and multiplayer validation.
// Missing optional model preserves the compiled default rig, but a corrupt
// model is a deployment error and is rejected rather than partially applied.
void loadBakedCharacterModels(const std::filesystem::path& assetRoot) {
    AssetPack pack(assetRoot);
    constexpr const char* file="models/characters.vlrm";
    if (!pack.contains(file)) {std::cout<<"Character models: built-in defaults\n";return;}
    const auto bytes=pack.read(file);
    std::array<SkeletonProfile,6> decoded{};
    std::string error;
    if(!runtime_model::decode(bytes,decoded,&error))
        throw std::runtime_error("Invalid bundled character models: "+error);
    loadActionSequences(pack,"models/characters.actions.json",decoded);
    characterSkeletons=decoded;
    loadCharacterMetadata(pack);
    std::cout<<"Character models: 6 baked rigs loaded from game.vpak\n";
}
void loadBakedEnemyModels(const std::filesystem::path& assetRoot) {
    AssetPack pack(assetRoot);
    constexpr const char* file="models/enemies.vlem";
    if(!pack.contains(file)){std::cout<<"Enemy models: built-in defaults\n";return;}
    const auto bytes=pack.read(file);
    std::array<SkeletonProfile,10> decoded{};
    std::string error;
    if(!enemy_runtime_model::decode(bytes,decoded,&error))
        throw std::runtime_error("Invalid bundled enemy motion models: "+error);
    loadActionSequences(pack,"models/enemies.actions.json",decoded);
    enemySkeletons=decoded;
    loadEnemyMetadata(pack);
    std::cout<<"Enemy models: 10 baked rigs loaded from game.vpak\n";
}
}
