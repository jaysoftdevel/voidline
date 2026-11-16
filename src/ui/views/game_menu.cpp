#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::gameMenu() {
    shell("orbital");
    r.rect(screen,hex(0x050d16).alpha(.40f));
    const Rect panel{285,95,630,407};
    r.gradient(panel,hex(0x1c3241),hex(0x0d1b27));
    r.outline(panel,hex(0x668396),1.5f);
    r.rect({panel.x,panel.y,6,panel.h},amber);
    label({316,135},"FIELD OPERATIONS / GAME MENU",amber);
    r.text({314,194},"Game menu",48,white,true);
    copy({316,223},"Continue where you left off, adjust settings, or return to the main menu.",555,16,pale);
    button({315,253,270,43},"CONTINUE",[this]{closeAction();},true,true);
    button({615,253,270,43},"SETTINGS",[this]{settingsReturn=View::gameMenu; show(View::settings);});
    const bool campaignReady=slot>0 || debugLoadoutReady;
    if (coopStarted) {
        button({315,308,270,43},"LEAVE SESSION",[this]{leaveCoop();});
        button({615,308,270,43},"MAIN MENU",[this]{leaveCoop(); show(View::menu);});
    } else {
        button({315,308,270,43},"SECTOR MAP",[this]{showMap();},campaignReady);
        button({615,308,270,43},"WEAPONS / SKILLS",[this]{showArmory();},campaignReady);
    }
    button({315,363,270,43},"MAIN MENU",[this]{
        if (coopStarted) leaveCoop();
        show(View::menu);
    });
    button({615,363,270,43},"QUIT GAME",[this]{
        ask(View::gameMenu,"Quit VOIDLINE?","Unsaved mission progress will be lost.",
            [this]{running=false;});
    });
    label({316,458},"ESC  /  RETURN TO PREVIOUS SCREEN",muted);
}
} // namespace vl
