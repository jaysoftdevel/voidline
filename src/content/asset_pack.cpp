#include "content/asset_pack.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace vl {
namespace {
constexpr uint64_t key=0x9D4F2DC37AB610E5ull;
uint64_t seed(std::string_view path) {
    uint64_t h=14695981039346656037ull;
    for (const unsigned char c:path) { h^=c; h*=1099511628211ull; }
    return h^key;
}
void unscramble(std::vector<uint8_t>& data,std::string_view name) {
    uint64_t state=seed(name),word=0;
    for (size_t i=0;i<data.size();++i) {
        if (!(i%8)) {
            state+=0x9E3779B97F4A7C15ull;
            uint64_t z=state;
            z=(z^(z>>30))*0xBF58476D1CE4E5B9ull;
            z=(z^(z>>27))*0x94D049BB133111EBull;
            word=z^(z>>31);
        }
        data[i]^=static_cast<uint8_t>(word>>(8*(i%8)));
    }
}
uint32_t checksum(std::span<const uint8_t> bytes) {
    uint32_t h=2166136261u;
    for (const uint8_t c:bytes) { h^=c; h*=16777619u; }
    return h;
}
uint32_t readNumber(const std::vector<uint8_t>& bytes,size_t& offset,int size) {
    if (offset>bytes.size() || bytes.size()-offset<static_cast<size_t>(size))
        throw std::runtime_error("Damaged VOIDLINE asset index.");
    uint32_t result=0;
    for (int i=0;i<size;++i) result|=uint32_t(bytes[offset++])<<(i*8);
    return result;
}
}
AssetPack::AssetPack(const std::filesystem::path& root) {
    const auto file=root/"game.vpak";
    std::ifstream stream(file,std::ios::binary);
    if (!stream) throw std::runtime_error("Asset bundle missing: "+file.string());
    archive.assign(std::istreambuf_iterator<char>(stream),{});
    if (archive.size()<12 || archive.size()>256*1024*1024 ||
        !std::equal(archive.begin(),archive.begin()+8,"VLPAK007"))
        throw std::runtime_error("Invalid VOIDLINE asset bundle.");
    size_t cursor=8;
    const uint32_t count=readNumber(archive,cursor,4);
    if (count==0 || count>2048) throw std::runtime_error("Asset bundle index exceeds limits.");
    for (uint32_t i=0;i<count;++i) {
        const uint32_t nameLength=readNumber(archive,cursor,2);
        if (nameLength==0 || nameLength>255 || archive.size()-cursor<nameLength)
            throw std::runtime_error("Invalid asset path in bundle.");
        std::string name(reinterpret_cast<const char*>(archive.data()+cursor),nameLength);
        cursor+=nameLength;
        if (name[0]=='/' || name.find("..")!=std::string::npos || name.find('\\')!=std::string::npos)
            throw std::runtime_error("Unsafe asset path in bundle.");
        const uint32_t size=readNumber(archive,cursor,4),sum=readNumber(archive,cursor,4);
        if (size>64*1024*1024 || size>archive.size()-cursor || !index.emplace(name,Entry{cursor,size,sum}).second)
            throw std::runtime_error("Damaged asset entry: "+name);
        cursor+=size;
    }
    if (cursor!=archive.size()) throw std::runtime_error("Trailing bytes in asset bundle.");
}
bool AssetPack::contains(std::string_view name) const { return index.find(std::string(name))!=index.end(); }
std::vector<uint8_t> AssetPack::read(std::string_view name) const {
    const auto entry=index.find(std::string(name));
    if (entry==index.end()) throw std::runtime_error("Missing bundled resource: "+std::string(name));
    const auto& e=entry->second;
    std::vector<uint8_t> result(archive.begin()+e.position,archive.begin()+e.position+e.length);
    unscramble(result,name);
    if (checksum(result)!=e.checksum) throw std::runtime_error("Corrupt bundled resource: "+std::string(name));
    return result;
}
}
