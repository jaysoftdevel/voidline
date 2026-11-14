#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::options() {
    shell("orbital"); heading("SYSTEM","Settings","Changes are saved as you make them.");
    auto setting=[&](float y,std::string name,std::string value,std::function<void()> action) {
        r.rect({55,y,525,51},hex(0x152531)); r.outline({55,y,525,51},hex(0x415565));
        r.text({70,y+31},name,17,white,true);
        button({385,y+7,179,36},std::move(value),std::move(action));
    };
    auto volume=[&](float y,std::string name,float& gain) {
        r.rect({615,y,530,51},hex(0x152531)); r.outline({615,y,530,51},hex(0x415565));
        r.text({630,y+31},name,17,white,true);
        r.text({988,y+32},std::to_string(int(std::round(gain*100)))+"%",15,white);
        auto* value=&gain;
        button({941,y+7,34,36},"-",[this,value]{*value=clamp(*value-.10f); commitSettings();});
        button({1098,y+7,33,36},"+",[this,value]{*value=clamp(*value+.10f); commitSettings();});
    };
    setting(181,"Game audio",settings.audio?"ON":"OFF",[this]{settings.audio=!settings.audio; commitSettings();});
    setting(242,"Screen shake and camera motion",settings.motion?"ON":"OFF",[this]{settings.motion=!settings.motion; commitSettings();});
    setting(303,"Opening cinematic",settings.cinematic?"ON":"OFF",[this]{settings.cinematic=!settings.cinematic; commitSettings();});
    setting(364,"New campaign difficulty",difficulties[settings.difficulty].name,[this]{settings.difficulty=(settings.difficulty+1)%3; commitSettings();});
    setting(425,"Lighting and bloom",settings.high?"HIGH":"PERFORMANCE",[this]{settings.high=!settings.high; commitSettings();});
    volume(181,"Music",settings.music);
    volume(242,"Effects",settings.effects);
    const bool fs=(SDL_GetWindowFlags(window)&sdl::fullscreen)!=0;
    r.text({630,333},"Window mode",17,white,true);
    button({941,308,190,37},fs?"FULLSCREEN":"WINDOWED",[this]{changeFullscreen();});
    r.rect({615,364,530,51},hex(0x152531)); r.outline({615,364,530,51},hex(0x415565));
    r.text({630,395},"Keyboard, sprint and crouch",17,white,true);
    button({941,371,190,37},"CONTROLS",[this]{rebinding=-1; show(View::controls);});
    r.rect({615,425,530,51},hex(0x152531)); r.outline({615,425,530,51},hex(0x415565));
    r.text({630,456},"Input diagnostics (F3)",17,white,true);
    button({941,432,190,37},settings.diagnostics?"ON":"OFF",[this]{settings.diagnostics=!settings.diagnostics; commitSettings();});
    button({55,526,142,41},"BACK",[this]{show(settingsReturn);},true,true);
}
} // namespace vl
