#pragma once
#include <cstdint>
#include <functional>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace art {
namespace fs=std::filesystem;
struct Raster {
    int width=0,height=0;
    std::vector<uint8_t> rgba;
    explicit operator bool()const{return width>0&&height>0&&rgba.size()==size_t(width)*size_t(height)*4;}
};
// Readable authoring resources are held outside the distributable game.
// game.vpak is read-only to this editor; saving goes to project/assets.
using Progress=std::function<void(int,int,const std::string&)>;
bool unpack(const fs::path& bundle,const fs::path& destination,int& count,std::string& error,
            Progress progress={});
// Rebuild a game-compatible VLPAK007 archive from a complete authoring asset tree.
// Writes to a temporary file, validates it, and retains a backup of the old pack.
bool repack(const fs::path& assets,const fs::path& destination,int& count,std::string& error,
            Progress progress={});
std::vector<std::string> files(const fs::path& directory);
bool decode(const fs::path& file,Raster& result,std::string& error);
bool encode(const fs::path& file,const Raster& source,std::string& error);
bool isImage(const std::string& name);
bool safePath(const fs::path& path);
void brush(Raster& image,int x,int y,int radius,uint8_t r,uint8_t g,uint8_t b,bool erase);
}
