#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::overlay() {
    r.rect(screen,hex(0x070b12).alpha(.72f));
    const Rect panel{300,151,600,291};
    r.gradient(panel,hex(0x172b38),hex(0x0f1d27));
    r.outline(panel,hex(0x5b7480));
    r.rect({panel.x,panel.y,5,panel.h},amber);
    const bool pause=view==View::pause, fail=view==View::failed;
    label({323,181},"SECTOR "+digits(game.index+1)+"  /  "+game.stage->name,amber);
    const std::string title=pause?"Paused":fail?"Suit integrity lost":game.index==sectorCount-1?"Campaign complete":"Sector cleared";
    r.text({321,240},title,48,white,true);
    if (pause) {
        r.text({325,277},"A / D  Move     SPACE  Jump (twice to flip)     SHIFT  Sprint     C  Crouch",15,pale);
        r.text({325,299},"Mouse / F  Fire     Q / Right click  Bullet time     R  Reload     1-7 / E / Wheel  Equip",15,pale);
        button({325,327,163,41},"RESUME",[this]{closeAction();},true,true);
        button({519,327,163,41},"SETTINGS",[this]{settingsReturn=View::pause; show(View::settings);});
        button({712,327,163,41},"LEAVE MISSION",[this] {
            ask(View::pause,"Leave this mission?","Returning to the sector map ends the current attempt.",[this]{showMap();});
        });
    } else {
        copy({325,274},fail?(game.activeBeacon>=0?"Retry from the active checkpoint.":"Retry the sector from its insertion point."):
            game.index==sectorCount-1?"The orbital uplink is online. Vega-9 can call for help again.":"Spend your earned score on upgrades in the armory.",534,17);
        if (!fail) label({325,321},std::to_string(game.score)+" SCORE   /   "+std::to_string(game.kills)+" TARGETS   /   "+timeText(game.time),teal);
        button({325,348,246,42},fail?"RETRY":"ARMORY",[this,fail]{if (fail) retry(); else showArmory();},true,true);
        button({624,348,251,42},"SECTOR MAP",[this]{showMap();});
    }
}
} // namespace vl
