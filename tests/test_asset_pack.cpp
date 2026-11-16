#include "support.hpp"
#include "content/asset_pack.hpp"
#include "game/detail.hpp"
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
    // Every firearm has its own firing texture; impacts distinguish surfaces.
    for (const char* effect : {"pistol","carbine","smg","shotgun","arc","launcher","railgun",
                               "enemy","guardian","hit-metal","hit-concrete","hit-rock"}) {
        for (int variation=1;variation<=4;++variation) {
            const std::string file=std::string("audio/")+effect+
                (variation==1?"":"-"+std::to_string(variation))+".ogg";
            CHECK(pack.contains(file));
            const auto bytes=pack.read(file);
            CHECK(bytes.size()>1500 && std::string(bytes.begin(),bytes.begin()+4)=="OggS");
        }
    }
    // The user's launcher recording contains a separate projectile detonation.
    // Its sample must be packaged independently from the firing report.
    // Distinct grounded takeoff and fabric-only midair flip sounds.
    for (const char* jump : {"jump.ogg","jump-2.ogg","jump-3.ogg","jump-air.ogg","jump-air-2.ogg"}) {
        const auto file=std::string("audio/")+jump;
        CHECK(pack.contains(file));
        const auto bytes=pack.read(file);
        CHECK(bytes.size()>1500 && std::string(bytes.begin(),bytes.begin()+4)=="OggS");
    }
    CHECK(pack.contains("audio/launcher-impact.ogg"));
    const auto launcherImpact=pack.read("audio/launcher-impact.ogg");
    CHECK(launcherImpact.size()>1500 && std::string(launcherImpact.begin(),launcherImpact.begin()+4)=="OggS");
    for (const char* name : {"pistol","carbine","smg","shotgun","arc","launcher","railgun"}) {
        CHECK(pack.contains(std::string("audio/reload-")+name+".ogg"));
    }
    CHECK(std::string(impactSound(Platform{{}, {}, true},Biome::mine))=="hit-rock");
    CHECK(std::string(impactSound(Platform{{}, {}, true},Biome::water))=="hit-concrete");
    CHECK(std::string(impactSound(Platform{{}, {}, false},Biome::water))=="hit-metal");
    CHECK(std::string(impactSound(Platform{{}, {}, false, nullptr, true},Biome::biodome))=="hit-concrete");
    // Distinct physical boot impacts for material types and left/right gait variation.
    for (const char* material : {"deck", "grate", "rock"}) {
        for (int variation=1;variation<=4;++variation) {
            const std::string name=std::string("audio/boot-")+material+
                (variation==1?"":"-"+std::to_string(variation))+".ogg";
            CHECK(pack.contains(name));
            const auto wav=pack.read(name);
            CHECK(wav.size()>1500 && std::string(wav.begin(),wav.begin()+4)=="OggS");
        }
    }
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
