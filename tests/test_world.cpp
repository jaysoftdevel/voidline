// Background events.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void ambientEvents() {
    using namespace vl;
    Silent audio;
    for (int sector = 0; sector < sectorCount; ++sector) {
        Game game(audio);
        game.start(sector, startSave());
        game.entry = 0;
        bool seen = false;
        for (int i = 0; i < 120 * 40; ++i) {
            game.tick();
            seen = seen || !game.ambient.empty();
            CHECK(game.ambient.size() <= 6);
        }
        CHECK(seen);
    }
}
}

void runWorldTests() {
    ambientEvents();
}
