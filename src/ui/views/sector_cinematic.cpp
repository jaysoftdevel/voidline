#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"

using namespace vl::ui;

namespace vl {
void App::sectorCinematic() {
    // Story page is deliberately one *static* pre-rendered environment picture.
    // No operative drawing, moving camera, particle animation, or time-dependent layers.
    const auto& chapter=story::chapters[game.index];
    const auto& stage=*game.stage;
    const auto accent=hex(stage.accent);
    const auto backdrop=story::scenery(chapter.set);
    r.rect(screen,hex(0x061018));
    r.cover(backdrop,{29,69,726,455},1.05f,{.53f,.49f},white);
    r.gradient({29,69,726,455},hex(0x061018).alpha(.02f),hex(0x061018).alpha(.42f));
    r.outline({29,69,726,455},hex(0x58717e));
    r.rect({29,22,1143,31},hex(0x102230));
    label({48,43},"VOIDLINE  /  STORY ARCHIVE  /  SECTOR "+digits(game.index+1),accent);
    r.rect({29,523,726,49},hex(0x102230));
    r.text({47,556},std::string(stage.name)+"  /  "+std::string(chapter.reveal),16,pale,true);

    const Rect panel{772,69,399,503};
    r.gradient(panel,hex(0x162936),hex(0x0a1823));
    r.outline(panel,hex(0x516a77));
    r.rect({772,69,4,503},accent);
    label({799,102},"CHAPTER REPORT",accent);
    r.text({797,146},chapter.title,26,white,true);
    r.line({797,161},{1146,161},1,hex(0x3a5364));
    float y=188;
    for (const auto& line:chapter.lines) {
        const std::string_view speaker=line.voice==story::Voice::lead?"OPERATIVE":
            line.voice==story::Voice::partner?"TEAM COMMS":
            line.voice==story::Voice::archive?"HELIX SYSTEM LOG":
            line.voice==story::Voice::aster?"RESCUE CUTTER ASTER":"NERI / CIVILIAN COMMS";
        r.text({799,y},speaker,12,accent,true);
        copy({799,y+23},line.text,344,16,pale);
        y+=121;
    }
    label({48,590},"PAGE 1 OF 2  /  STORY",muted);
    if (!coopStarted || network.hosting())
        button({902,539,263,35},"LEVEL SUMMARY",[this]{skipEpilogue();},true,true);
    else label({819,590},"WAITING FOR HOST",teal);
}
}
