#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::load() {
    shell("orbital"); heading("SAVED CAMPAIGNS","Load game","Select a campaign to return to the sector map.");
    for (int i=0;i<3;++i) {
        const Rect box{55,178.f+i*107,1090,92};
        r.gradient(box,hex(0x172a36),hex(0x111e29));
        r.outline(box,hex(0x465a67));
        label({72,box.y+22},"SLOT "+digits(i+1),amber);
        if (slots[i]) {
            const auto& s=*slots[i];
            r.text({71,box.y+52},characters[s.character].name,27,white,true);
            r.text({72,box.y+74},std::string(difficulties[s.difficulty].name)+"    |    "+
                std::to_string(s.cleared)+"/5 sectors    |    "+std::to_string(s.score)+" total score",14,pale);
        } else r.text({72,box.y+59},"Empty slot",22,muted,true);
        button({948,box.y+27,180,39},"LOAD",[this,i]{loadCampaign(i+1);},slots[i].has_value(),true);
    }
    button({55,524,150,40},"MAIN MENU",[this]{show(View::menu);});
}
} // namespace vl
