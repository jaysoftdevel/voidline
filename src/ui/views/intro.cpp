#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::cinematic() {
    struct Cut { const char *art,*tag,*text; float at,bias; };
    constexpr Cut cuts[]{
        {"orbital","VEGA-9  /  LOW ORBIT","The colony went silent.",0,.42f},
        {"descent","MINING SECTOR  /  SIGNAL LOST","Its defenses stayed online.",3.8f,.52f},
        {"cover","FIELD OPERATIONS  /  INSERTION","Reach the orbital uplink.",7.55f,.55f},
        {"terminus","FIELD OPERATIONS","VOIDLINE",11.18f,.43f}
    };
    int i=0;
    while (i<3 && film>=cuts[i+1].at) ++i;
    const auto& c=cuts[i];
    const float t=film-c.at;
    const float opacity=smooth(t/.6f);
    if (i>0 && opacity<1) r.cover(cuts[i-1].art,screen,settings.motion?1.10f:1.f,{.55f,cuts[i-1].bias});
    r.cover(c.art,screen,settings.motion?1.02f+t*.019f:1.f,{.55f,c.bias},white.alpha(opacity));
    r.gradient(screen,hex(0x060b14).alpha(.14f),hex(0x060b14).alpha(.81f));
    r.rect({0,0,width,24},hex(0x050911)); r.rect({0,576,width,24},hex(0x050911));
    label({59,443},c.tag,amber.alpha(opacity));
    r.text({55,516},c.text,i==3?92:54,white.alpha(opacity),true);
    button({1021,520,124,39},"SKIP INTRO",[this]{launch(0);});
}
} // namespace vl
