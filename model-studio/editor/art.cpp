#include "art.hpp"
#include <png.h>
#include <webp/decode.h>
#include <webp/encode.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <limits>
#include <span>

namespace art {
namespace {
constexpr uint64_t magicKey=0x9D4F2DC37AB610E5ull;
uint32_t fnv(std::span<const uint8_t> data){
    uint32_t h=2166136261u;for(auto c:data){h^=c;h*=16777619u;}return h;
}
uint64_t seed(std::string_view name){
    uint64_t h=14695981039346656037ull;
    for(unsigned char c:name){h^=c;h*=1099511628211ull;}return h^magicKey;
}
void unscramble(std::vector<uint8_t>& bytes,std::string_view name){
    uint64_t state=seed(name),word=0;
    for(size_t i=0;i<bytes.size();++i){
        if(i%8==0){state+=0x9E3779B97F4A7C15ull;
            uint64_t z=state;z=(z^(z>>30))*0xBF58476D1CE4E5B9ull;
            z=(z^(z>>27))*0x94D049BB133111EBull;word=z^(z>>31);}
        bytes[i]^=uint8_t(word>>(8*(i%8)));
    }
}
std::string lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=char(c+32);return s;}
bool read(std::span<const uint8_t> data,size_t& i,int bytes,uint32_t& value){
    if(i>data.size()||data.size()-i<size_t(bytes))return false;
    value=0;for(int k=0;k<bytes;++k)value|=uint32_t(data[i++])<<(8*k);return true;
}
}
bool safePath(const fs::path& path){
    if(path.empty()||path.is_absolute()||path.has_root_path())return false;
    for(const auto& component:path)if(component==".."||component=="."||component.empty())return false;
    return true;
}
bool unpack(const fs::path& bundle,const fs::path& directory,int& count,std::string& error,Progress progress){
    count=0;error.clear();
    std::ifstream stream(bundle,std::ios::binary);
    if(!stream){error="Cannot open game.vpak";return false;}
    std::vector<uint8_t> input((std::istreambuf_iterator<char>(stream)),{});
    if(input.size()<12||input.size()>256u*1024u*1024u||
       !std::equal(input.begin(),input.begin()+8,"VLPAK007")){
        error="Invalid game.vpak header";return false;}
    size_t pos=8;uint32_t total=0;
    if(!read(input,pos,4,total)||!total||total>2048){error="Invalid archive resource count";return false;}
    // Parse/check the complete archive before writing any files.
    struct Entry {fs::path name;std::vector<uint8_t> contents;};
    std::vector<Entry> entries;entries.reserve(total);
    for(uint32_t k=0;k<total;++k){
        uint32_t len=0,size=0,expected=0;
        if(!read(input,pos,2,len)||len==0||len>255||len>input.size()-pos){error="Malformed asset name";return false;}
        const std::string name(reinterpret_cast<const char*>(input.data()+pos),len);pos+=len;
        if(name.find('\\')!=std::string::npos||name.find(':')!=std::string::npos||!safePath(fs::path(name))){error="Unsafe asset path";return false;}
        if(!read(input,pos,4,size)||!read(input,pos,4,expected)||size>64u*1024u*1024u||size>input.size()-pos){error="Invalid asset payload";return false;}
        std::vector<uint8_t> bytes(input.begin()+static_cast<long>(pos),input.begin()+static_cast<long>(pos+size));pos+=size;
        unscramble(bytes,name);
        if(fnv(bytes)!=expected){error="Asset checksum mismatch: "+name;return false;}
        if(std::any_of(entries.begin(),entries.end(),[&](const Entry& e){return e.name==fs::path(name);})){error="Duplicate asset name";return false;}
        entries.push_back({name,std::move(bytes)});
    }
    if(pos!=input.size()){error="Trailing archive data";return false;}
    for(const auto& entry:entries){
        const auto target=directory/entry.name;
        std::error_code ec;fs::create_directories(target.parent_path(),ec);
        if(ec){error="Cannot create asset directory";return false;}
        std::ofstream output(target,std::ios::binary|std::ios::trunc);
        if(!output||!output.write(reinterpret_cast<const char*>(entry.contents.data()),static_cast<std::streamsize>(entry.contents.size()))){error="Cannot write "+target.string();return false;}
        ++count;
        if(progress)progress(count,int(total),entry.name.generic_string());
    }
    return true;
}
static void writeNum(std::ostream& stream,uint32_t n,int bytes) {
    for(int i=0;i<bytes;++i)stream.put(char((n>>(8*i))&255u));
}
bool repack(const fs::path& assets,const fs::path& destination,int& count,std::string& error,Progress progress) {
    count=0;error.clear();
    if(!fs::is_directory(assets)){error="Authoring assets directory is missing";return false;}
    std::vector<fs::path> entries;
    std::error_code ec;
    for(fs::recursive_directory_iterator it(assets,fs::directory_options::none,ec),end;it!=end&&!ec;it.increment(ec)){
        if(it->is_symlink(ec)){error="Symlinks are not permitted in the asset tree";return false;}
        if(!it->is_regular_file(ec))continue;
        auto name=it->path().lexically_relative(assets);
        if(!safePath(name)||name.extension()==".vpak"||name.begin()->string()=="licenses")continue;
        if(name.generic_string().size()>255){error="Asset path is too long";return false;}
        if(fs::file_size(it->path(),ec)>64u*1024u*1024u){error="Asset exceeds 64 MiB";return false;}
        entries.push_back(std::move(name));
    }
    if(ec){error="Cannot enumerate authoring assets";return false;}
    std::sort(entries.begin(),entries.end());
    if(entries.empty()||entries.size()>2048){error="Empty or excessive asset catalog";return false;}
    fs::create_directories(destination.parent_path(),ec);
    if(ec){error="Cannot create pack destination";return false;}
    const fs::path temp=destination.string()+".writing";
    fs::remove(temp,ec);ec.clear();
    {
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);
        if(!out){error="Cannot create temporary asset pack";return false;}
        out.write("VLPAK007",8);writeNum(out,uint32_t(entries.size()),4);
        for(const auto& name:entries){
            const auto path=assets/name;
            std::ifstream source(path,std::ios::binary);
            if(!source){error="Cannot read asset: "+name.generic_string();break;}
            std::vector<uint8_t> payload((std::istreambuf_iterator<char>(source)),{});
            const std::string key=name.generic_string();
            if(payload.size()>64u*1024u*1024u){error="Asset exceeds 64 MiB";break;}
            writeNum(out,uint32_t(key.size()),2);out.write(key.data(),std::streamsize(key.size()));
            writeNum(out,uint32_t(payload.size()),4);writeNum(out,fnv(payload),4);
            unscramble(payload,key);
            out.write(reinterpret_cast<const char*>(payload.data()),std::streamsize(payload.size()));
            if(!out){error="Cannot write packed asset";break;}
            ++count;
            if(progress)progress(count,int(entries.size()),key);
        }
        out.flush();
        if(!out)error="Asset pack write failed";
    }
    if(!error.empty()){fs::remove(temp,ec);return false;}
    // Verify the full pack and all per-entry checksums before replacing the game file.
    // Unpack into a throwaway directory to ensure our output matches the runtime format.
    const fs::path check=destination.string()+".verify";
    fs::remove_all(check,ec);ec.clear();
    int validated=0;std::string why;
    const bool good=unpack(temp,check,validated,why);
    fs::remove_all(check,ec);ec.clear();
    if(!good||validated!=count){error="New archive verification failed: "+why;fs::remove(temp,ec);return false;}
    if(fs::file_size(temp,ec)>256u*1024u*1024u||ec){error="Pack size exceeds game limit";fs::remove(temp,ec);return false;}
    fs::path backup;
    if(fs::exists(destination)) {
        auto timestamp=std::chrono::system_clock::now().time_since_epoch().count();
        backup=destination.string()+".backup-"+std::to_string(timestamp);
        fs::rename(destination,backup,ec);
        if(ec){error="Cannot back up original game.vpak";fs::remove(temp,ec);return false;}
    }
    fs::rename(temp,destination,ec);
    if(ec){
        if(!backup.empty()){
            std::error_code restore;fs::rename(backup,destination,restore);
        }
        error="Cannot install new archive; original restored";fs::remove(temp,ec);return false;
    }
    return true;
}
std::vector<std::string> files(const fs::path& directory){
    std::vector<std::string> result;
    if(!fs::is_directory(directory))return result;
    for(const auto& item:fs::recursive_directory_iterator(directory))if(item.is_regular_file()){
        const auto rel=item.path().lexically_relative(directory).generic_string();
        if(safePath(rel))result.push_back(rel);
    }
    std::sort(result.begin(),result.end());return result;
}
bool isImage(const std::string& name){
    const auto ext=lower(fs::path(name).extension().string());
    return ext==".webp"||ext==".png";
}
bool decode(const fs::path& filename,Raster& result,std::string& error){
    std::ifstream file(filename,std::ios::binary);
    if(!file){error="Asset not found";return false;}
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    if(bytes.empty()||bytes.size()>64u*1024u*1024u){error="Invalid media size";return false;}
    Raster next{};
    const auto ext=lower(filename.extension().string());
    if(ext==".webp"){
        if(!WebPGetInfo(bytes.data(),bytes.size(),&next.width,&next.height)){
            error="Invalid WebP image";return false;}
        if(next.width<=0||next.height<=0||next.width>8192||next.height>8192){error="Image dimension limit exceeded";return false;}
        next.rgba.resize(size_t(next.width)*next.height*4);
        if(!WebPDecodeRGBAInto(bytes.data(),bytes.size(),next.rgba.data(),next.rgba.size(),next.width*4)){
            error="WebP decode failed";return false;}
    }else if(ext==".png"){
        png_image image{};image.version=PNG_IMAGE_VERSION;
        if(!png_image_begin_read_from_memory(&image,bytes.data(),bytes.size())){error="PNG header decode failed";return false;}
        if(image.width==0||image.height==0||image.width>8192||image.height>8192){png_image_free(&image);error="Image dimension limit exceeded";return false;}
        next.width=int(image.width);next.height=int(image.height);image.format=PNG_FORMAT_RGBA;
        next.rgba.resize(PNG_IMAGE_SIZE(image));
        if(!png_image_finish_read(&image,nullptr,next.rgba.data(),0,nullptr)){
            png_image_free(&image);error="PNG decode failed";return false;}
        png_image_free(&image);
    }else{error="Only PNG and WebP pictures can be painted";return false;}
    result=std::move(next);return true;
}
bool encode(const fs::path& filename,const Raster& image,std::string& error){
    if(!image){error="No valid image to export";return false;}
    std::error_code ec;fs::create_directories(filename.parent_path(),ec);
    if(ec){error="Cannot create output asset directory";return false;}
    const auto temp=fs::path(filename.string()+".tmp");
    const auto ext=lower(filename.extension().string());
    if(ext==".webp"){
        uint8_t* buffer=nullptr;
        const size_t length=WebPEncodeLosslessRGBA(image.rgba.data(),image.width,image.height,image.width*4,&buffer);
        if(length==0||!buffer){error="WebP encode failed";return false;}
        std::ofstream output(temp,std::ios::binary|std::ios::trunc);
        if(!output||!output.write(reinterpret_cast<const char*>(buffer),std::streamsize(length))){WebPFree(buffer);error="Cannot write edited WebP";return false;}
        WebPFree(buffer);
    }else if(ext==".png"){
        png_image png{};png.version=PNG_IMAGE_VERSION;png.width=unsigned(image.width);png.height=unsigned(image.height);png.format=PNG_FORMAT_RGBA;
        if(!png_image_write_to_file(&png,temp.string().c_str(),0,image.rgba.data(),0,nullptr)){
            error="PNG encode failed";return false;}
    }else{error="Unsupported export format";return false;}
    // Replacing an asset is an explicit offline operation and never modifies game.vpak.
    fs::rename(temp,filename,ec);
    if(ec){ec.clear();fs::remove(filename,ec);ec.clear();fs::rename(temp,filename,ec);}
    if(ec){error="Cannot replace edited asset";return false;}
    return true;
}
void brush(Raster& image,int px,int py,int radius,uint8_t r,uint8_t g,uint8_t b,bool erase){
    if(!image)return;
    radius=std::clamp(radius,1,60);
    for(int y=std::max(0,py-radius);y<std::min(image.height,py+radius+1);++y)
        for(int x=std::max(0,px-radius);x<std::min(image.width,px+radius+1);++x){
            const int dx=x-px,dy=y-py;
            if(dx*dx+dy*dy>radius*radius)continue;
            auto* c=image.rgba.data()+(size_t(y)*image.width+x)*4;
            if(erase){c[0]=0;c[1]=0;c[2]=0;c[3]=0;}
            else{c[0]=r;c[1]=g;c[2]=b;c[3]=255;}
        }
}
} // namespace art
