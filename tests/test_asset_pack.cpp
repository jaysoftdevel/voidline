#include "support.hpp"
#include "content/asset_pack.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace vl;
using namespace test;

void runAssetPackTests() {
    const std::filesystem::path root=VOIDLINE_TEST_ASSET_ROOT;
    CHECK(std::filesystem::is_regular_file(root/"game.vpak"));
    CHECK(!std::filesystem::exists(root/"audio/menu.ogg"));
    CHECK(!std::filesystem::exists(root/"cover.webp"));
    AssetPack pack(root);
    CHECK(pack.contains("audio/menu.ogg"));
    CHECK(pack.contains("audio/coop-intro.ogg"));
    CHECK(pack.contains("fonts/font-regular.png"));
    CHECK(pack.contains("fonts/font-regular.txt"));
    CHECK(pack.contains("cinematic/deck.webp"));
    CHECK(pack.contains("nyx.webp"));
    CHECK(!pack.contains("audio/nonexistent.ogg"));
    const auto audio=pack.read("audio/menu.ogg");
    const auto image=pack.read("cover.webp");
    const auto font=pack.read("fonts/font-regular.png");
    CHECK(audio.size()>8 && std::string(audio.begin(),audio.begin()+4)=="OggS");
    CHECK(image.size()>12 && std::string(image.begin(),image.begin()+4)=="RIFF");
    CHECK(font.size()>8 && font[0]==0x89 && font[1]=='P' && font[2]=='N' && font[3]=='G');
    bool threw=false;
    try { pack.read("missing.png"); }
    catch (const std::runtime_error&) { threw=true; }
    CHECK(threw);
    // The first media payload is not a playable file inside the container.
    std::ifstream in(root/"game.vpak",std::ios::binary);
    std::vector<uint8_t> archive(std::istreambuf_iterator<char>{in},{});
    CHECK(archive.size()>100 && std::string(archive.begin(),archive.begin()+8)=="VLPAK007");
}
