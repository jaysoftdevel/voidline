#include "ui/app.hpp"
#include "ui/common.hpp"
#include <array>

using namespace vl::ui;

namespace vl {
void App::graphicsOptions() {
    shell("orbital");
    heading("VIDEO", "Graphics complexity", "Adjust individual rendering passes. Changes apply immediately.");
    const Color dim=hex(0x9bb4c2), border=hex(0x486071);
    button({53,159,235,38},"CORE RENDERING",[this]{graphicsPage=0;},true,graphicsPage==0);
    button({301,159,235,38},"EFFECTS & WORLD",[this]{graphicsPage=1;},true,graphicsPage==1);
    const auto& q=settings.graphics;
    static constexpr std::array<const char*,5> presetNames{"PERFORMANCE","BALANCED","QUALITY","ULTRA","CUSTOM"};
    r.text({700,184},"ACTIVE PROFILE: "+std::string(presetNames[q.preset]),15,teal,true);

    struct Control { const char* name; const char* hint; int GraphicsQuality::* field; int max; bool off; };
    static constexpr std::array<Control,6> core{{
        {"Internal resolution","50 / 67 / 80 / 100 / 115%",&GraphicsQuality::resolution,4,false},
        {"Local lighting","Light count and light-map detail",&GraphicsQuality::lighting,3,false},
        {"Cast shadows","Occlusion, soft contact shadows",&GraphicsQuality::shadows,3,true},
        {"Material relief","Surface texture and detail",&GraphicsQuality::materials,3,false},
        {"Anti-aliasing","FXAA sampling detail",&GraphicsQuality::antialias,2,true},
        {"Color grading","Contrast and cinematic tint",&GraphicsQuality::grading,3,true}
    }};
    static constexpr std::array<Control,8> effects{{
        {"Bloom","Glow spread and blur passes",&GraphicsQuality::bloom,3,true},
        {"Emissive glow","Brightness from light sources",&GraphicsQuality::glow,3,true},
        {"Atmospheric fog","Haze and light scattering",&GraphicsQuality::fog,3,true},
        {"Reflections","Reflection pass resolution",&GraphicsQuality::reflections,3,true},
        {"Water effects","Refraction and wave details",&GraphicsQuality::water,3,false},
        {"Volumetric light shafts","Light cones in the atmosphere",&GraphicsQuality::shafts,3,true},
        {"Particles","Particle and background event count",&GraphicsQuality::particles,3,true},
        {"Scenery density","Foreground and distance details",&GraphicsQuality::scenery,3,false}
    }};
    constexpr std::array<const char*,4> levels{"MINIMAL","LOW","HIGH","ULTRA"};
    constexpr std::array<const char*,4> optional{"OFF","LOW","HIGH","ULTRA"};
    constexpr std::array<const char*,5> resolutions{"50%","67%","80%","100%","115%"};
    auto row=[&](int position,std::string name,std::string help,std::string value,std::function<void()> action) {
        const float x=position>=4?615.f:53.f;
        const float y=207.f+(position%4)*67.f;
        r.rect({x,y,532,60},hex(0x10212c).alpha(.94f));
        r.outline({x,y,532,60},border);
        r.text({x+12,y+24},name,17,white,true);
        r.text({x+12,y+46},help,12,dim);
        button({x+349,y+10,170,39},std::move(value),std::move(action));
    };
    if (graphicsPage==0) {
        row(0,"Graphics preset","Sets all rendering options",presetNames[q.preset],[this]{
            settings.graphics=graphicsPreset(settings.graphics.preset==4?0:(settings.graphics.preset+1)%4);
            commitSettings();
        });
        row(1,"Dynamic resolution","Adapts to available frame time",q.dynamicResolution?"ON":"OFF",[this]{
            settings.graphics.dynamicResolution=!settings.graphics.dynamicResolution;
            settings.graphics.preset=4; commitSettings();
        });
        for (size_t i=0;i<core.size();++i) {
            const Control& c=core[i];
            const int value=q.*(c.field);
            const std::string label=c.field==&GraphicsQuality::resolution?resolutions[value]:
                c.field==&GraphicsQuality::antialias?optional[value]:
                c.off?optional[value]:levels[value];
            // The final world-detail control is also present on the effects page for discoverability.
            const int position=int(i)+2;
            if (position>=8) continue;
            row(position,c.name,c.hint,label,[this,field=c.field,max=c.max]{
                auto& setting=settings.graphics.*field;
                setting=(setting+1)%(max+1);
                settings.graphics.preset=4; commitSettings();
            });
        }
    } else {
        for (size_t i=0;i<effects.size();++i) {
            const Control& c=effects[i];
            const int value=q.*(c.field);
            const std::string label=c.off?optional[value]:levels[value];
            row(int(i),c.name,c.hint,label,[this,field=c.field,max=c.max]{
                auto& setting=settings.graphics.*field;
                setting=(setting+1)%(max+1);
                settings.graphics.preset=4; commitSettings();
            });
        }
    }
    r.text({55,499},"Visual settings are local; gameplay and multiplayer synchronization are unchanged.",14,dim);
    button({55,535,145,39},"BACK",[this]{show(View::settings);},true,true);
    button({994,535,150,39},"RESET QUALITY",[this]{settings.graphics=graphicsPreset(2); commitSettings();});
}
} // namespace vl
