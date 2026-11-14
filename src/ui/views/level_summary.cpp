#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"

using namespace vl::ui;

namespace vl {
void App::levelSummary() {
    const auto& stage=*game.stage;
    const auto& chapter=story::chapters[game.index];
    const auto accent=hex(stage.accent);
    r.cover(stage.art,screen,1.06f,{.56f,.48f},white);
    r.rect(screen,hex(0x061019).alpha(.83f));
    r.rect({32,26,1136,543},hex(0x101e2a).alpha(.94f));
    r.outline({32,26,1136,543},hex(0x536b7b));
    r.rect({32,26,5,543},accent);
    label({68,65},"PAGE 2 OF 2  /  MISSION SUMMARY",accent);
    r.text({66,114},game.index==sectorCount-1?"Campaign complete":"Sector complete",44,white,true);
    r.text({68,149},std::string(stage.name)+"  /  "+std::string(chapter.reveal),17,pale);
    const std::array<std::pair<std::string,std::string>,4> stats{{
        {"MISSION SCORE",std::to_string(game.score)},
        {"TARGETS DEFEATED",std::to_string(game.kills)},
        {"COMPLETION TIME",timeText(game.time)},
        {"SECTOR PROGRESS",std::to_string(game.index+1)+" / "+std::to_string(sectorCount)}
    }};
    for (int n=0;n<4;++n) {
        const float x=67+(n%2)*550.f,y=189+(n/2)*116.f;
        r.gradient({x,y,517,98},hex(0x1a3240),hex(0x102431));
        r.outline({x,y,517,98},hex(0x47606e));
        label({x+20,y+27},stats[n].first,teal);
        r.text({x+19,y+70},stats[n].second,30,white,true);
    }
    r.text({68,473},"Story complete. Review your result before proceeding.",17,pale);
    if (coopStarted) {
        if (network.hosting() && game.index+1<sectorCount)
            button({68,501,280,46},"NEXT SECTOR",[this]{startCoop(game.index+1);},true,true);
        else if (!network.hosting()) label({69,527},"WAITING FOR HOST",teal);
        button({879,501,254,46},"LEAVE SESSION",[this]{leaveCoop();});
    } else {
        button({68,501,281,46},"ARMORY",[this]{showArmory();},true,true);
        button({879,501,254,46},"SECTOR MAP",[this]{showMap();});
    }
}
}
