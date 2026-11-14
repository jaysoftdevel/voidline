#include "support.hpp"
#include "render/figures/gun_art.hpp"
#include <set>
#include <string_view>

using namespace vl;
using namespace vl::figures;
using namespace test;

void runWeaponArtTests() {
    CHECK(gunArt.size()==weaponCount);
    std::set<std::string_view> seen;
    for (size_t i=0;i<gunArt.size();++i) {
        const GunArt& art=gunArt[i];
        CHECK(art.name && *art.name);
        CHECK(seen.insert(art.name).second);
        CHECK(art.length>20.f && art.length<90.f);
        CHECK(art.aspect>.15f && art.aspect<.65f);
        CHECK(art.muzzleV>.2f && art.muzzleV<.65f);
        CHECK(art.gripU>.20f && art.gripU<.55f);
        CHECK(art.gripV>.50f && art.gripV<.90f);
        CHECK(art.foreU>art.gripU && art.foreU<.82f);
        CHECK(art.foreV>.45f && art.foreV<.90f);
    }
    CHECK(std::string_view(gunArt[3].name)=="pistol");
    CHECK(std::string_view(gunArt[4].name)=="smg");
    CHECK(std::string_view(gunArt[5].name)=="launcher");
    CHECK(std::string_view(gunArt[6].name)=="railgun");
}
