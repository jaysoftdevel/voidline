#include "source_export.hpp"
#include "content/stages.hpp"
#include "content/level_io.hpp"
#include "content/model_io.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_runtime_model.hpp"
#include <array>
#include <json-c/json.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <system_error>

namespace source_export {
namespace fs=std::filesystem;
namespace {
std::string quoted(const char* text){std::ostringstream o;o<<std::quoted(text?text:"");return o.str();}
std::string f(float n){
    std::ostringstream o;o<<std::setprecision(9)<<n;
    std::string s=o.str();
    if(s.find_first_of(".eE")==std::string::npos)s+=".0";
    return s+"f";
}
std::string v(vl::Vec p){return "{"+f(p.x)+","+f(p.y)+"}";}
std::string rect(vl::Rect r){return "{"+f(r.x)+","+f(r.y)+","+f(r.w)+","+f(r.h)+"}";}
template<class Item,class Func> void list(std::ostream& out,const char* name,const std::vector<Item>& items,Func format){
    out<<"        s."<<name<<"={";
    for(size_t i=0;i<items.size();++i){if(i)out<<",";if(i%4==0)out<<"\n          ";out<<format(items[i]);}
    out<<"};\n";
}
bool writeText(const fs::path& target,const std::string& value,std::string& error){
    std::error_code ec;fs::create_directories(target.parent_path(),ec);
    if(ec){error="Cannot create generated C++ directory";return false;}
    std::ofstream stream(target,std::ios::binary|std::ios::trunc);
    if(!stream||!(stream<<value)){error="Cannot write "+target.string();return false;}
    return true;
}
bool writeBinary(const fs::path& file,const std::vector<uint8_t>& data,std::string& error){
    std::error_code ec;fs::create_directories(file.parent_path(),ec);
    if(ec){error="Cannot create binary model directory";return false;}
    std::ofstream out(file,std::ios::binary|std::ios::trunc);
    if(!out||!out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()))){
        error="Cannot write baked VLRM model";return false;
    }
    return true;
}
std::string actionCatalog(const fs::path& project,bool enemy,std::string& error){
    json_object* root=json_object_new_object(),*items=json_object_new_array();
    json_object_object_add(root,"schema",json_object_new_string("voidline-actions-v1"));
    json_object_object_add(root,"models",items);
    // Retain the editable descriptions in the generated catalog. They are not
    // a statement that the game runtime plays all these poses yet.
    json_object* descriptor=json_object_from_file((project/"models/movement-descriptions.json").string().c_str());
    json_object* definitions=nullptr;
    if(descriptor && json_object_object_get_ex(descriptor,"movements",&definitions) &&
       json_object_is_type(definitions,json_type_array))
        json_object_object_add(root,"definitions",json_object_get(definitions));
    const int count=enemy?int(vl::specs.size()):int(vl::characters.size());
    for(int i=0;i<count;++i){
        const fs::path file=enemy?vl::enemyModelFile(project/"models",i):vl::characterModelFile(project/"models",i);
        json_object* parsed=json_object_from_file(file.string().c_str());
        if(!parsed){error="Could not parse action source: "+file.string();json_object_put(root);
                    if(descriptor)json_object_put(descriptor);
                    return {};
        }
        json_object *skeleton=nullptr,*layers=nullptr,*depths=nullptr,*frames=nullptr;
        if(json_object_object_get_ex(parsed,"skeleton",&skeleton)){
            json_object_object_get_ex(skeleton,"sequences",&layers);
            json_object_object_get_ex(skeleton,"sequenceDepth",&depths);
            json_object_object_get_ex(skeleton,"poseFrames",&frames);
        }
        json_object* allLayers=layers&&json_object_is_type(layers,json_type_object)?
                               json_object_get(layers):json_object_new_object();
        json_object* allDepths=depths&&json_object_is_type(depths,json_type_object)?
                               json_object_get(depths):json_object_new_object();
        // Export precisely the action layers stored in this individual model.
        // The shared descriptor is metadata, not a request to synthesize poses.
        json_object* entry=json_object_new_object();
        json_object_object_add(entry,"id",json_object_new_int(i));
        json_object_object_add(entry,"sequences",allLayers);
        json_object_object_add(entry,"sequenceDepth",allDepths);
        // Frame tracks are literal editable source data. The exporter never
        // invents, pads or replaces authored motion.
        json_object_object_add(entry,"poseFrames",frames&&json_object_is_type(frames,json_type_object)?
                                    json_object_get(frames):json_object_new_object());
        json_object_array_add(items,entry);
        json_object_put(parsed);
    }
    std::string result=json_object_to_json_string_ext(root,JSON_C_TO_STRING_PRETTY);
    json_object_put(root);if(descriptor)json_object_put(descriptor);return result+"\n";
}
std::string generatedStages(const std::vector<vl::Stage>& stages,bool coop){
    std::ostringstream out;
    out<<"// Generated offline by VOIDLINE Model Studio v0.0.17. Replace this original source manually.\n"
       <<"#include \"content/stages.hpp\"\n#include <utility>\nnamespace vl {\n"
       <<"const std::array<Stage,"<<(coop?"cooperativeSectorCount":"sectorCount")<<"> "
       <<(coop?"cooperativeStages":"stages")<<"=[] {\n"
       <<"    std::array<Stage,"<<(coop?"cooperativeSectorCount":"sectorCount")<<"> campaign{};\n";
    for(size_t index=0;index<stages.size();++index){
        const auto& s=stages[index];
        out<<"    {\n        Stage s{};\n";
        for(auto pair:std::array<std::pair<const char*,const char*>,8>{{
            {"name",s.name},{"art",s.art},{"location",s.location},{"threat",s.threat},
            {"accent",s.accent},{"cold",s.cold},{"briefing",s.briefing},{"boss",s.boss}}})
            out<<"        s."<<pair.first<<"="<<quoted(pair.second)<<";\n";
        out<<"        s.biome=static_cast<Biome>("<<int(s.biome)<<");\n"
           <<"        s.worldWidth="<<f(s.worldWidth)<<";\n"
           <<"        s.exit="<<v(s.exit)<<";\n"
           <<"        s.insertion="<<v(s.insertion)<<";\n";
        list(out,"ground",s.ground,rect);
        list(out,"ledges",s.ledges,rect);
        list(out,"cover",s.cover,rect);
        list(out,"beacons",s.beacons,v);
        list(out,"stairs",s.stairs,[](vl::Stair t){return "{"+v(t.from)+","+v(t.to)+"}";});
        list(out,"ladders",s.ladders,[](vl::Ladder l){return "{"+f(l.x)+","+f(l.top)+","+f(l.bottom)+"}";});
        list(out,"lifts",s.lifts,[](vl::Lift l){return "{"+f(l.x)+","+f(l.y)+","+f(l.w)+","+f(l.dx)+","+f(l.dy)+","+f(l.period)+","+f(l.phase)+"}";});
        list(out,"vents",s.vents,[](vl::Vent t){return "{"+f(t.x)+","+f(t.y)+","+f(t.phase)+"}";});
        list(out,"enemies",s.enemies,[](vl::EnemyPlacement e){return "{static_cast<EnemyKind>("+std::to_string(int(e.kind))+"),"+f(e.x)+","+f(e.y)+","+f(e.patrol)+","+(e.elite?"true":"false")+"}";});
        list(out,"supplies",s.supplies,[](vl::SupplyPlacement t){return "{static_cast<SupplyKind>("+std::to_string(int(t.kind))+"),"+f(t.x)+","+f(t.y)+"}";});
        list(out,"crates",s.crates,[](vl::CratePlacement t){return "{static_cast<CrateKind>("+std::to_string(int(t.kind))+"),"+v(t.base)+"}";});
        list(out,"relays",s.relays,[](vl::RelayGate g){return "{"+rect(g.barrier)+","+v(g.pad)+","+std::to_string(g.operatorPlayer)+",static_cast<RelayControl>("+std::to_string(int(g.control))+")}";});
        out<<"        s.structurePolygons={";
        for(const auto& poly:s.structurePolygons){
            out<<"{ "<<poly.layer<<","<<poly.index<<",{";
            for(const auto& point:poly.corners)out<<v(point)<<",";
            out<<"}},";
        }
        out<<"};\n";
        out<<"        campaign["<<index<<"]=std::move(s);\n    }\n";
    }
    out<<"    return campaign;\n}();\n}\n";
    return out.str();
}
// Enemy motion is exported independently: original enemy stats are unchanged.
std::string generatedEnemyMotion(){
    std::ostringstream out;
    out<<"// Generated by VOIDLINE Model Studio. Use with the enemy rig renderer integration.\n"
       <<"#pragma once\n#include \"content/skeleton_types.hpp\"\nnamespace vl {\n"
       <<"inline std::array<SkeletonProfile,10> enemySkeletons{{\n";
    for(const auto& profile:vl::enemySkeletons){
        out<<"    SkeletonProfile{ {\n";
        for(const auto& layer:profile.offsets){
            out<<"        std::array<Vec,skeletonJointCount>{{";
            for(size_t j=0;j<layer.size();++j){if(j)out<<",";out<<v(layer[j]);}
            out<<"}},\n";
        }
        out<<"    }, {\n";
        for(const auto& layer:profile.depths){
            out<<"        std::array<float,skeletonJointCount>{{";
            for(size_t j=0;j<layer.size();++j){if(j)out<<",";out<<f(layer[j]);}
            out<<"}},\n";
        }
        out<<"    }, "<<f(profile.stride)<<","<<f(profile.armSwing)<<","<<f(profile.aimRaise)
           <<","<<f(profile.bodyHeight)<<","<<f(profile.bodyWidth)<<","<<f(profile.headScale)
           <<","<<f(profile.armLength)<<","<<f(profile.legLength)<<","<<f(profile.weaponSeat)
           <<","<<f(profile.walkCycle)<<","<<f(profile.acceleration)<<","<<f(profile.braking)
           <<","<<f(profile.recoil)<<","<<f(profile.idleBreath)<<"},\n";
    }
    out<<"}};\n} // namespace vl\n";
    return out.str();
}
// Immutable runtime metadata bundled next to the baked geometry. This keeps
// editable names and character statistics live without a game recompilation.
std::string characterMetadata(){
    json_object* root=json_object_new_array();
    for(size_t i=0;i<vl::characters.size();++i){
        const auto& c=vl::characters[i];
        json_object* o=json_object_new_object();
        json_object_object_add(o,"id",json_object_new_int(static_cast<int>(i)));
        for(auto [key,value]:std::array<std::pair<const char*,const char*>,6>{{
            {"name",c.name},{"role",c.role},{"art",c.art},{"color",c.color},
            {"trait",c.trait},{"description",c.description}}})
            json_object_object_add(o,key,json_object_new_string(value));
        json_object_object_add(o,"health",json_object_new_int(c.health));
        json_object_object_add(o,"speed",json_object_new_double(c.speed));
        json_object_object_add(o,"jump",json_object_new_double(c.jump));
        json_object_object_add(o,"ammo",json_object_new_double(c.ammo));
        json_object_object_add(o,"rig",json_object_new_int(c.rig));
        json_object_array_add(root,o);
    }
    std::string result=json_object_to_json_string_ext(root,JSON_C_TO_STRING_PRETTY);
    json_object_put(root);
    return result+"\n";
}
std::string enemyMetadata(){
    json_object* root=json_object_new_array();
    for(size_t i=0;i<vl::specs.size();++i){
        const auto& e=vl::specs[i];
        json_object* o=json_object_new_object();
        json_object_object_add(o,"id",json_object_new_int(static_cast<int>(i)));
        json_object_object_add(o,"name",json_object_new_string(vl::enemyNames[i]));
        for(auto [key,value]:std::array<std::pair<const char*,double>,5>{{
            {"width",e.w},{"height",e.h},{"speed",e.speed},{"reach",e.reach},{"attackDelay",e.delay}}})
            json_object_object_add(o,key,json_object_new_double(value));
        for(auto [key,value]:std::array<std::pair<const char*,int>,3>{{
            {"health",e.hp},{"damage",e.damage},{"score",e.score}}})
            json_object_object_add(o,key,json_object_new_int(value));
        json_object_array_add(root,o);
    }
    std::string result=json_object_to_json_string_ext(root,JSON_C_TO_STRING_PRETTY);
    json_object_put(root);return result+"\n";
}

std::string generatedCharacters(){
    std::ostringstream out;
    out<<"// Generated offline by VOIDLINE Model Studio v0.0.17. Replace original manually.\n"
       <<"#pragma once\n#include \"content/types.hpp\"\nnamespace vl {\n"
       <<"inline std::array<Character,6> characters{{\n";
    for(const auto& c:vl::characters){
        out<<"    {"<<quoted(c.name)<<","<<quoted(c.role)<<","<<quoted(c.art)<<","<<quoted(c.color)
           <<","<<quoted(c.trait)<<","<<quoted(c.description)<<","<<c.health<<","<<f(c.speed)<<","<<f(c.jump)<<","<<f(c.ammo)<<","<<c.rig<<"},\n";
    }
    out<<"}};\ninline const std::array<Difficulty,3> difficulties{{\n";
    for(const auto& d:vl::difficulties)
        out<<"    {"<<quoted(d.name)<<","<<f(d.hp)<<","<<f(d.damage)<<","<<f(d.pace)<<","<<f(d.score)<<"},\n";
    out<<"}};\n} // namespace vl\n";return out.str();
}
// Compiled, offline replacement for the procedural game rig. No runtime JSON loading.
std::string generatedSkeleton(){
    std::ostringstream out;
    out<<"// Generated by VOIDLINE Model Studio 0.0.16. Copy into src/content/ before rebuilding.\n"
          "#pragma once\n#include \"content/skeleton_types.hpp\"\nnamespace vl {\n"
          "inline std::array<SkeletonProfile,6> characterSkeletons{{\n";
    for(const auto& profile:vl::characterSkeletons){
        out<<"    SkeletonProfile{ {\n";
        for(const auto& layer:profile.offsets){
            out<<"        std::array<Vec,skeletonJointCount>{{";
            for(size_t j=0;j<layer.size();++j){if(j)out<<",";out<<v(layer[j]);}
            out<<"}},\n";
        }
        out<<"    }, {\n";
        for(const auto& layer:profile.depths){
            out<<"        std::array<float,skeletonJointCount>{{";
            for(size_t j=0;j<layer.size();++j){if(j)out<<",";out<<f(layer[j]);}
            out<<"}},\n";
        }
        out<<"    }, "<<f(profile.stride)<<","<<f(profile.armSwing)<<","<<f(profile.aimRaise)
           <<","<<f(profile.bodyHeight)<<","<<f(profile.bodyWidth)<<","<<f(profile.headScale)
           <<","<<f(profile.armLength)<<","<<f(profile.legLength)<<","<<f(profile.weaponSeat)
           <<","<<f(profile.walkCycle)<<","<<f(profile.acceleration)<<","<<f(profile.braking)
           <<","<<f(profile.recoil)<<","<<f(profile.idleBreath)<<"},\n";
    }
    out<<"}};\n} // namespace vl\n";
    return out.str();
}
} // namespace
bool generate(const fs::path& project,const fs::path& output,std::string& error){
    std::vector<vl::Stage> campaign,coop;
    for(int c=0;c<2;++c)for(int i=0;i<(c?vl::cooperativeSectorCount:vl::sectorCount);++i){
        const auto& original=c?vl::cooperativeStages[i]:vl::stages[i];
        vl::Stage loaded{};
        if(!vl::readLevel(vl::levelFile(project/"levels",i,c!=0),original,loaded,i,c!=0,&error))
            return false;
        (c?coop:campaign).push_back(std::move(loaded));
    }
    for(int i=0;i<int(vl::characters.size());++i)if(!vl::loadCharacterModel(project/"models",i,&error))return false;
    for(int i=0;i<int(vl::specs.size());++i)if(!vl::loadEnemyModel(project/"models",i,&error))return false;
    const auto folder=output/"src"/"content";
    try{
        const auto packed=vl::runtime_model::encode(vl::characterSkeletons);
        if(!writeBinary(output/"assets"/"models"/"characters.vlrm",packed,error))return false;
        const auto enemyPacked=vl::enemy_runtime_model::encode(vl::enemySkeletons);
        if(!writeBinary(output/"assets"/"models"/"enemies.vlem",enemyPacked,error))return false;
        if(!writeText(output/"assets"/"models"/"characters.info.json",characterMetadata(),error))return false;
        if(!writeText(output/"assets"/"models"/"enemies.info.json",enemyMetadata(),error))return false;
        const auto characters=actionCatalog(project,false,error);
        const auto enemies=actionCatalog(project,true,error);
        if(characters.empty()||enemies.empty())return false;
        if(!writeText(output/"assets"/"models"/"characters.actions.json",characters,error) ||
           !writeText(output/"assets"/"models"/"enemies.actions.json",enemies,error))return false;
    }catch(const std::exception& e){error=e.what();return false;}
    return writeText(folder/"stages.cpp",generatedStages(campaign,false),error)&&
           writeText(folder/"cooperative.cpp",generatedStages(coop,true),error)&&
           writeText(folder/"characters.hpp",generatedCharacters(),error)&&
           writeText(folder/"skeleton.hpp",generatedSkeleton(),error)&&
           writeText(folder/"enemy_motion.hpp",generatedEnemyMotion(),error);
}
} // namespace source_export
