#include "ui/app.hpp"
#include "ui/common.hpp"
#include "core/version.hpp"

using namespace vl::ui;

namespace vl {
void App::menu() {
    r.cover("cover",screen,1.035f,{.62f,.52f},hex(0xd6e4f3));
    r.gradient(screen,hex(0x0e1722).alpha(.25f),hex(0x090f18).alpha(.83f));
    r.gradient({0,0,568,height},hex(0x070f18).alpha(.95f),hex(0x080f19).alpha(.88f));
    r.rect({0,0,6,height},hex(0xe49a64));
    r.line({563,0},{563,height},1.1f,amber.alpha(.31f));
    r.image("voidline-logo.png",{51,42,70,70});
    label({135,77},"VEGA-9  /  COLONY RESCUE",amber);
    r.text({49,179},"VOIDLINE",96,white,true);
    r.rect({54,204,65,3},amber);
    copy({54,241},"A dead colony. One operative. Fight your way to the orbital uplink.",360,20,pale);
    button({54,282,339,39},"NEW GAME",[this]{newGame();},true,true);
    button({54,330,339,39},"LOAD GAME",[this]{refresh(); show(View::load);});
    button({54,378,339,39},"MULTIPLAYER",[this]{openMultiplayer();});
    button({54,426,339,39},"SETTINGS",[this]{settingsReturn=View::menu; show(View::settings);});
    button({54,474,339,39},"CREDITS",[this]{show(View::credits);});
    button({54,522,339,39},"QUIT",[this]{running=false;});
    if (settings.debugUnlockLevels) {
        r.gradient({688,354,457,152},hex(0x203542),hex(0x101c28));
        r.outline({688,354,457,152},amber.alpha(.65f));
        label({707,380},"DEBUG MODE / SESSION LOADOUT",amber);
        button({706,397,203,41},"LEVEL SELECTION",[this]{openDebug(false);},true,true);
        button({922,397,204,41},"WEAPONS / UPGRADES",[this]{openDebug(true);},true,true);
        copy({707,468},"Test every sector and cycle upgrade ranks without using credits or saving purchases.",415,12.5f,pale);
    }
    label({1003,566},std::string("NATIVE EDITION  /  ")+version,pale);
}
} // namespace vl
