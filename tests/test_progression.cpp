// Upgrades, perks and campaign data.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void upgradesAndPerks() {
    using namespace vl;
    Silent audio;
    Game base(audio), tuned(audio);
    base.start(0, fullSave());
    Save save = fullSave();
    for (size_t g = 0; g < weaponCount; ++g) save.tiers[g] = {3, 3, 3, 3, 3, 3, 3, 3};
    save.perks = {4, 4, 4, 4, 4, 4};
    tuned.start(0, save);
    for (size_t g = 0; g < weaponCount; ++g) {
        CHECK(base.guns[g].available && tuned.guns[g].available);
        CHECK(tuned.guns[g].damage > base.guns[g].damage);
        CHECK(tuned.guns[g].range > base.guns[g].range * 1.4f);
        CHECK(tuned.guns[g].interval < base.guns[g].interval);
        CHECK(tuned.guns[g].capacity > base.guns[g].capacity);
        CHECK(tuned.guns[g].spread <= base.guns[g].spread);
        CHECK(tuned.guns[g].crit > 0.f);
    }
    CHECK(tuned.maxHealth > base.maxHealth);
    CHECK(tuned.player.hp == tuned.maxHealth);
    CHECK(tuned.slowDrain < base.slowDrain && tuned.slowRegen > base.slowRegen);
    CHECK(tuned.damageTaken < base.damageTaken);
    // Every original gun reaches farther than before (760, 365, 970 and 680).
    CHECK(weapons[0].range > 760 && weapons[1].range > 365 && weapons[2].range > 970 && weapons[3].range > 680);
}
void stageData() {
    using namespace vl;
    for (const auto& stage : stages) {
        CHECK(stage.worldWidth > stage.exit.x);
        CHECK(!stage.ground.empty());
        CHECK(!stage.enemies.empty() && stage.enemies.back().kind == EnemyKind::boss);
    }
}
}

void runProgressionTests() {
    upgradesAndPerks();
    stageData();
}
