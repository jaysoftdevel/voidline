#include "content/level_io.hpp"
#include "game/stairs.hpp"
#include "content/model_io.hpp"
#include <json-c/json.h>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <memory>
#include <system_error>
#include <limits>

namespace vl {
namespace {
using Ptr=std::unique_ptr<json_object,decltype(&json_object_put)>;
constexpr int maximumObjects=512;
bool fail(std::string* message,const char* why) { if (message) *message=why; return false; }
json_object* field(json_object* object,const char* key) {
    json_object* value=nullptr;
    if (!object || !json_object_is_type(object,json_type_object)) return nullptr;
    return json_object_object_get_ex(object,key,&value)?value:nullptr;
}
bool number(json_object* value,float& out,float low=-500.f,float high=200000.f) {
    if (!value || !(json_object_is_type(value,json_type_int)||json_object_is_type(value,json_type_double))) return false;
    const double d=json_object_get_double(value);
    if (!std::isfinite(d)||d<low||d>high) return false;
    out=static_cast<float>(d); return true;
}
bool integer(json_object* value,int& out,int low,int high) {
    if (!value || !(json_object_is_type(value,json_type_int)||json_object_is_type(value,json_type_double))) return false;
    const double n=json_object_get_double(value);
    if (!std::isfinite(n)||n<low||n>high||std::floor(n)!=n) return false;
    out=static_cast<int>(n);return true;
}
bool tuple(json_object* object,int count,std::array<float,8>& values) {
    if (!object||!json_object_is_type(object,json_type_array)||json_object_array_length(object)!=static_cast<size_t>(count)) return false;
    for (int i=0;i<count;++i) if (!number(json_object_array_get_idx(object,i),values[i])) return false;
    return true;
}
json_object* array(std::initializer_list<float> values) {
    json_object* j=json_object_new_array();
    for (float value:values) json_object_array_add(j,json_object_new_double(value));
    return j;
}
template<class T,class Parse>
bool readArray(json_object* root,const char* key,std::vector<T>& output,Parse parse) {
    json_object* j=field(root,key);
    if (!j||!json_object_is_type(j,json_type_array)||json_object_array_length(j)>maximumObjects) return false;
    output.clear();output.reserve(json_object_array_length(j));
    for (size_t i=0;i<json_object_array_length(j);++i) {
        T value{};
        if (!parse(json_object_array_get_idx(j,i),value)) return false;
        output.push_back(value);
    }
    return true;
}
template<class T,class Encode>
void putArray(json_object* root,const char* key,const std::vector<T>& values,Encode encode) {
    json_object* result=json_object_new_array();
    for (const auto& value:values) json_object_array_add(result,encode(value));
    json_object_object_add(root,key,result);
}
bool parseRect(json_object* j,Rect& r) {
    std::array<float,8> a{};
    if (!tuple(j,4,a)) return false;
    r={a[0],a[1],a[2],a[3]};
    return r.w>=4 && r.h>=4 && r.w<=40000 && r.h<=500;
}
bool parseVec(json_object* j,Vec& p) {
    std::array<float,8> a{}; if (!tuple(j,2,a)) return false;
    p={a[0],a[1]};return true;
}
}
std::filesystem::path levelFile(const std::filesystem::path& directory,int sector,bool cooperative) {
    const int n=sector+1;
    const std::string name=std::string(cooperative?"coop-":"sector-")+(n<10?"0":"")+std::to_string(n)+".vllevel.json";
    return directory/name;
}

bool readLevel(const std::filesystem::path& file,const Stage& base,Stage& result,
               int sector,bool cooperative,std::string* error) {
    std::ifstream stream(file,std::ios::binary);
    if (!stream) return fail(error,"Cannot open level file.");
    const std::string source((std::istreambuf_iterator<char>(stream)),{});
    if (source.size()>4*1024*1024) return fail(error,"Level exceeds 1 MiB limit.");
    Ptr root(json_tokener_parse(source.c_str()),json_object_put);
    if (!root||!json_object_is_type(root.get(),json_type_object)) return fail(error,"Invalid JSON.");
    auto* schema=field(root.get(),"schema");
    if (!schema||!json_object_is_type(schema,json_type_string) ||
        std::string_view(json_object_get_string(schema))!="voidline-level-v1") return fail(error,"Unsupported level format.");
    int id=-1,mode=-1;
    if (!integer(field(root.get(),"sector"),id,1,sectorCount) || id!=sector+1 ||
        !integer(field(root.get(),"cooperative"),mode,0,1)||mode!=int(cooperative))
        return fail(error,"Level mode or sector number does not match.");
    Stage edit=base;
    if (json_object* insertion=field(root.get(),"insertion"); insertion && !parseVec(insertion,edit.insertion))
        return fail(error,"Invalid operative spawn location.");
    if (!number(field(root.get(),"worldWidth"),edit.worldWidth,1200.f,90000.f))
        return fail(error,"Invalid world width.");
    if (!parseVec(field(root.get(),"exit"),edit.exit)) return fail(error,"Invalid exit.");
    const auto rects=[](json_object* j,Rect& r) { return parseRect(j,r); };
    const auto points=[](json_object* j,Vec& p) { return parseVec(j,p); };
    if (!readArray(root.get(),"ground",edit.ground,rects)||edit.ground.empty()||
        !readArray(root.get(),"ledges",edit.ledges,rects)||
        !readArray(root.get(),"cover",edit.cover,rects)||
        !readArray(root.get(),"beacons",edit.beacons,points))
        return fail(error,"Malformed terrain or checkpoints.");
    if (!readArray(root.get(),"stairs",edit.stairs,[](json_object* j,Stair& s) {
            std::array<float,8> a{};if (!tuple(j,4,a)) return false;
            s={{a[0],a[1]},{a[2],a[3]}};return validStair(s);
        })) return fail(error,"Invalid staircase endpoints.");
    if (field(root.get(),"ladders") &&
        !readArray(root.get(),"ladders",edit.ladders,[](json_object* j,Ladder& l) {
            std::array<float,8> a{}; if (!tuple(j,3,a)) return false;
            l={a[0],a[1],a[2]};
            return l.x>=0.f && l.top>=-500.f && l.bottom>l.top+45.f && l.bottom<2000.f;
        })) return fail(error,"Invalid climbable ladder.");
    if (!readArray(root.get(),"lifts",edit.lifts,[](json_object* j,Lift& l) {
            std::array<float,8> a{};if (!tuple(j,7,a)) return false;
            l={a[0],a[1],a[2],a[3],a[4],a[5],a[6]};
            return l.w>=24 && l.w<=600 && l.period>=.5f && l.period<=30.f;
        })) return fail(error,"Invalid moving lift.");
    if (!readArray(root.get(),"vents",edit.vents,[](json_object* j,Vent& v) {
            std::array<float,8> a{};if (!tuple(j,3,a)) return false;
            v={a[0],a[1],a[2]};return true;
        })) return fail(error,"Invalid vent.");
    if (!readArray(root.get(),"enemies",edit.enemies,[](json_object* j,EnemyPlacement& e) {
            if (!json_object_is_type(j,json_type_array)||json_object_array_length(j)!=5) return false;
            int kind=0,elite=0;
            if (!integer(json_object_array_get_idx(j,0),kind,0,int(EnemyKind::juggernaut)) ||
                !number(json_object_array_get_idx(j,1),e.x) ||
                !number(json_object_array_get_idx(j,2),e.y) ||
                !number(json_object_array_get_idx(j,3),e.patrol,0,1000) ||
                !integer(json_object_array_get_idx(j,4),elite,0,1)) return false;
            e.kind=static_cast<EnemyKind>(kind);e.elite=elite!=0;return true;
        })) return fail(error,"Invalid enemy placements.");
    if (edit.enemies.size()>128) return fail(error,"More than 128 enemies exceed multiplayer snapshot limits.");
    if (!readArray(root.get(),"supplies",edit.supplies,[](json_object* j,SupplyPlacement& s) {
            if (!json_object_is_type(j,json_type_array)||json_object_array_length(j)!=3) return false;
            int kind=0;
            if (!integer(json_object_array_get_idx(j,0),kind,0,int(SupplyKind::health)) ||
                !number(json_object_array_get_idx(j,1),s.x) ||
                !number(json_object_array_get_idx(j,2),s.y)) return false;
            s.kind=static_cast<SupplyKind>(kind);return true;
        })) return fail(error,"Invalid supply placements.");
    if (edit.supplies.size()>128) return fail(error,"More than 128 supplies exceed multiplayer snapshot limits.");
    if (!readArray(root.get(),"crates",edit.crates,[](json_object* j,CratePlacement& c) {
            if (!json_object_is_type(j,json_type_array)||json_object_array_length(j)!=3) return false;
            int kind=0;
            if (!integer(json_object_array_get_idx(j,0),kind,0,int(CrateKind::reactor)) ||
                !number(json_object_array_get_idx(j,1),c.base.x) ||
                !number(json_object_array_get_idx(j,2),c.base.y)) return false;
            c.kind=static_cast<CrateKind>(kind);return true;
        })) return fail(error,"Invalid crate placements.");
    if (edit.crates.size()>128) return fail(error,"More than 128 crates exceed multiplayer snapshot limits.");
    if (!readArray(root.get(),"relays",edit.relays,[](json_object* j,RelayGate& g) {
            if (!json_object_is_type(j,json_type_array)||json_object_array_length(j)!=8) return false;
            std::array<float,8> a{};if (!tuple(j,8,a)) return false;
            g={{a[0],a[1],a[2],a[3]},{a[4],a[5]},int(a[6]),static_cast<RelayControl>(int(a[7]))};
            return g.barrier.w>=16 && g.barrier.h>=32 &&
                (g.operatorPlayer==0||g.operatorPlayer==1) && (int(a[7])==0||int(a[7])==1);
        })) return fail(error,"Invalid cooperative relay.");
    if (edit.relays.size()>24) return fail(error,"More than 24 relay gates exceed multiplayer snapshot limits.");
    edit.structurePolygons.clear();
    if(auto* outlines=field(root.get(),"structureVertices")){
        if(!json_object_is_type(outlines,json_type_object) || json_object_object_length(outlines)>maximumObjects)
            return fail(error,"Invalid structure outline container");
        json_object_object_foreach(outlines,label,points) {
            const std::string key(label);
            const size_t separator=key.rfind(':');
            if(separator==std::string::npos)return fail(error,"Invalid polygon structure identifier");
            const auto group=key.substr(0,separator),number=key.substr(separator+1);
            int layer=-1;
            if(group=="ground")layer=0;
            else if(group=="ledges")layer=1;
            else if(group=="cover")layer=2;
            else if(group=="relays")layer=3;
            if(layer<0 || number.empty() || number.find_first_not_of("0123456789")!=std::string::npos)
                return fail(error,"Unknown polygon structure reference");
            int index=0;
            try{index=std::stoi(number);}catch(...){return fail(error,"Invalid structure index");}
            const size_t bound=layer==0?edit.ground.size():layer==1?edit.ledges.size():
                    layer==2?edit.cover.size():edit.relays.size();
            if(index<0 || static_cast<size_t>(index)>=bound || !json_object_is_type(points,json_type_array) ||
                json_object_array_length(points)<3 || json_object_array_length(points)>32)
                return fail(error,"Invalid polygon vertex count or reference");
            StructurePolygon polygon{};polygon.layer=layer;polygon.index=index;
            for(size_t i=0;i<json_object_array_length(points);++i){
                Vec vertex{};
                if(!parseVec(json_object_array_get_idx(points,i),vertex))
                    return fail(error,"Invalid polygon vertex coordinate");
                polygon.corners.push_back(vertex);
            }
            edit.structurePolygons.push_back(std::move(polygon));
        }
    }
    if (edit.exit.x<0 || edit.exit.x>edit.worldWidth || edit.insertion.x<0 ||
        edit.insertion.x>edit.worldWidth-48.f || edit.ground.front().x>100 ||
        edit.ground.front().w<200 ||
        std::any_of(edit.stairs.begin(),edit.stairs.end(),[&](const Stair& s) {return s.from.x<0||s.to.x>edit.worldWidth;}))
        return fail(error,"Invalid insertion point, exit, or staircase bounds.");
    result=std::move(edit);
    return true;
}

bool writeLevel(const std::filesystem::path& file,const Stage& s,int sector,
                bool cooperative,std::string* error) {
    if (sector<0||sector>=(cooperative?cooperativeSectorCount:sectorCount) || s.ground.empty())
        return fail(error,"Invalid stage selection.");
    Ptr root(json_object_new_object(),json_object_put);
    json_object_object_add(root.get(),"schema",json_object_new_string("voidline-level-v1"));
    json_object_object_add(root.get(),"sector",json_object_new_int(sector+1));
    json_object_object_add(root.get(),"cooperative",json_object_new_int(int(cooperative)));
    json_object_object_add(root.get(),"worldWidth",json_object_new_double(s.worldWidth));
    json_object_object_add(root.get(),"exit",array({s.exit.x,s.exit.y}));
    json_object_object_add(root.get(),"insertion",array({s.insertion.x>0?s.insertion.x:100.f,
        s.insertion.y==0?s.ground.front().y:s.insertion.y}));
    auto rect=[](Rect r) {return array({r.x,r.y,r.w,r.h});};
    auto vec=[](Vec v) {return array({v.x,v.y});};
    putArray(root.get(),"ground",s.ground,rect);
    putArray(root.get(),"ledges",s.ledges,rect);
    putArray(root.get(),"cover",s.cover,rect);
    putArray(root.get(),"beacons",s.beacons,vec);
    putArray(root.get(),"stairs",s.stairs,[](const Stair& st){return array({st.from.x,st.from.y,st.to.x,st.to.y});});
    putArray(root.get(),"ladders",s.ladders,[](const Ladder& l){return array({l.x,l.top,l.bottom});});
    putArray(root.get(),"lifts",s.lifts,[](const Lift& l){return array({l.x,l.y,l.w,l.dx,l.dy,l.period,l.phase});});
    putArray(root.get(),"vents",s.vents,[](const Vent& v){return array({v.x,v.y,v.phase});});
    putArray(root.get(),"enemies",s.enemies,[](const EnemyPlacement& e){return array({float(int(e.kind)),e.x,e.y,e.patrol,float(e.elite)});});
    putArray(root.get(),"supplies",s.supplies,[](const SupplyPlacement& v){return array({float(int(v.kind)),v.x,v.y});});
    putArray(root.get(),"crates",s.crates,[](const CratePlacement& c){return array({float(int(c.kind)),c.base.x,c.base.y});});
    putArray(root.get(),"relays",s.relays,[](const RelayGate& g){return array({g.barrier.x,g.barrier.y,g.barrier.w,g.barrier.h,g.pad.x,g.pad.y,float(g.operatorPlayer),float(int(g.control))});});
    json_object* outlines=json_object_new_object();
    static constexpr const char* names[]{"ground","ledges","cover","relays"};
    for(const auto& polygon:s.structurePolygons){
        if(polygon.layer<0 || polygon.layer>3 || polygon.corners.size()<3 || polygon.corners.size()>32)continue;
        const auto key=std::string(names[polygon.layer])+":"+std::to_string(polygon.index);
        json_object* vertices=json_object_new_array();
        for(const auto& point:polygon.corners)json_object_array_add(vertices,array({point.x,point.y}));
        json_object_object_add(outlines,key.c_str(),vertices);
    }
    json_object_object_add(root.get(),"structureVertices",outlines);
    const char* data=json_object_to_json_string_ext(root.get(),JSON_C_TO_STRING_PRETTY);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(),ec);
    if (ec) return fail(error,"Could not create level directory.");
    const auto temp=file.string()+".tmp";
    {std::ofstream stream(temp,std::ios::binary|std::ios::trunc);
        if (!stream || !(stream<<data<<'\n')) return fail(error,"Could not write level data.");}
    std::filesystem::rename(temp,file,ec);
    if (ec) { // Windows does not replace an existing destination on rename.
        ec.clear();std::filesystem::remove(file,ec);ec.clear();
        std::filesystem::rename(temp,file,ec);
    }
    return ec?fail(error,"Could not replace level file."):true;
}

uint32_t levelFingerprint(const Stage& s) {
    // Stable across Windows/Linux even when JSON was pretty-printed differently.
    uint32_t h=2166136261u;
    auto feed=[&](uint32_t n) {for(int i=0;i<4;++i){h^=(n>>(i*8))&255u;h*=16777619u;}};
    auto f=[&](float n){feed(std::bit_cast<uint32_t>(n));};
    auto vec=[&](Vec p){f(p.x);f(p.y);};
    auto rect=[&](Rect r){f(r.x);f(r.y);f(r.w);f(r.h);};
    f(s.worldWidth);vec(s.exit);vec({s.insertion.x>0?s.insertion.x:100.f,s.insertion.y==0?s.ground.front().y:s.insertion.y});
    feed(uint32_t(s.ground.size()));for(auto& a:s.ground)rect(a);
    feed(uint32_t(s.ledges.size()));for(auto& a:s.ledges)rect(a);
    feed(uint32_t(s.cover.size()));for(auto& a:s.cover)rect(a);
    feed(uint32_t(s.stairs.size()));for(auto& a:s.stairs){vec(a.from);vec(a.to);}
    feed(uint32_t(s.ladders.size()));for(auto& a:s.ladders){f(a.x);f(a.top);f(a.bottom);}
    feed(uint32_t(s.lifts.size()));for(auto& a:s.lifts){f(a.x);f(a.y);f(a.w);f(a.dx);f(a.dy);f(a.period);f(a.phase);}
    feed(uint32_t(s.enemies.size()));for(auto& a:s.enemies){feed(uint32_t(a.kind));f(a.x);f(a.y);f(a.patrol);feed(a.elite);}
    feed(uint32_t(s.beacons.size()));for(auto& a:s.beacons)vec(a);
    feed(uint32_t(s.supplies.size()));for(auto& a:s.supplies){feed(uint32_t(a.kind));f(a.x);f(a.y);}
    feed(uint32_t(s.crates.size()));for(auto& a:s.crates){feed(uint32_t(a.kind));vec(a.base);}
    feed(uint32_t(s.vents.size()));for(auto& a:s.vents){f(a.x);f(a.y);f(a.phase);}
    feed(uint32_t(s.relays.size()));for(auto& a:s.relays){rect(a.barrier);vec(a.pad);feed(uint32_t(a.operatorPlayer));feed(uint32_t(a.control));}
    feed(uint32_t(s.structurePolygons.size()));
    for(const auto& poly:s.structurePolygons){
        feed(uint32_t(poly.layer));feed(uint32_t(poly.index));feed(uint32_t(poly.corners.size()));
        for(auto point:poly.corners)vec(point);
    }
    feed(modelFingerprint());
    return h;
}
} // namespace vl
