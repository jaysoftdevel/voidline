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
    setting(159,"Game audio",settings.audio?"ON":"OFF",[this]{settings.audio=!settings.audio; commitSettings();});
    setting(215,"Screen shake and camera motion",settings.motion?"ON":"OFF",[this]{settings.motion=!settings.motion; commitSettings();});
    setting(271,"Opening cinematic",settings.cinematic?"ON":"OFF",[this]{settings.cinematic=!settings.cinematic; commitSettings();});
    setting(327,"New campaign difficulty",difficulties[settings.difficulty].name,[this]{settings.difficulty=(settings.difficulty+1)%3; commitSettings();});
    setting(383,"Lighting and bloom",settings.high?"HIGH":"PERFORMANCE",[this]{settings.high=!settings.high; commitSettings();});
    setting(439,"Show FPS counter",settings.fpsCounter?"ON":"OFF",[this]{settings.fpsCounter=!settings.fpsCounter; commitSettings();});
    volume(159,"Music",settings.music);
    volume(215,"Effects",settings.effects);
    const bool fs=(SDL_GetWindowFlags(window)&sdl::fullscreen)!=0;
    r.rect({615,271,530,51},hex(0x152531)); r.outline({615,271,530,51},hex(0x415565));
    r.text({630,302},"Window mode",17,white,true);
    button({941,278,190,37},fs?"FULLSCREEN":"WINDOWED",[this]{changeFullscreen();});
    r.rect({615,327,530,51},hex(0x152531)); r.outline({615,327,530,51},hex(0x415565));
    r.text({630,358},"Keyboard, sprint and crouch",17,white,true);
    button({941,334,190,37},"CONTROLS",[this]{rebinding=-1; show(View::controls);});
    r.rect({615,383,530,51},hex(0x152531)); r.outline({615,383,530,51},hex(0x415565));
    r.text({630,414},"Input diagnostics (F3)",17,white,true);
    button({941,390,190,37},settings.diagnostics?"ON":"OFF",[this]{settings.diagnostics=!settings.diagnostics; commitSettings();});
    static constexpr std::array<std::string_view,4> corners{"TOP LEFT","TOP RIGHT","BOTTOM LEFT","BOTTOM RIGHT"};
    r.rect({615,439,530,51},hex(0x152531)); r.outline({615,439,530,51},hex(0x415565));
    r.text({630,470},"FPS display corner",17,white,true);
    button({941,446,190,37},std::string(corners[settings.fpsCorner]),[this]{settings.fpsCorner=(settings.fpsCorner+1)%4; commitSettings();});
    button({55,526,142,41},"BACK",[this]{show(settingsReturn);},true,true);
}
} // namespace vl
