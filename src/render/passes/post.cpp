#include "render/renderer.hpp"

namespace vl {
void Renderer::post(const Game& g,bool motionFx) {
    bind(lit,false);
    gl.UseProgram(atmosphereProgram);
    useTexture(atmosphereProgram,"world",0,raw.texture.id);
    useTexture(atmosphereProgram,"lighting",1,lightMap.texture.id);
    const Color tint=hex(g.stage->cold);
    gl.Uniform3f(uniform(atmosphereProgram,"haze"),tint.r,tint.g,tint.b);
    gl.Uniform1f(uniform(atmosphereProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(atmosphereProgram,"time"),g.time);
    gl.Uniform1f(uniform(atmosphereProgram,"fogStrength"),(g.stage->biome==Biome::water || g.stage->biome==Biome::biodome?.72f:.55f)*(.0f+.46f*graphics.fog));
    gl.Uniform2f(uniform(atmosphereProgram,"lightTexel"),1.f/lightMap.texture.w,1.f/lightMap.texture.h);
    gl.Uniform2f(uniform(atmosphereProgram,"worldTexel"),1.f/raw.texture.w,1.f/raw.texture.h);
    gl.Uniform1f(uniform(atmosphereProgram,"relief"),graphics.materials==0?0.f:graphics.materials==1?3.5f:graphics.materials==2?7.5f:10.f);
    screen(atmosphereProgram);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    actors(g);
    flush();
    gl.Disable(GL_BLEND);

    const int levels=graphics.bloom==0?0:graphics.bloom==1?2:graphics.bloom==2?4:5;
    Texture from=lit.texture;
    if (!levels) bind(bloom[0],true);  // never expose a stale bloom buffer on quality changes
    for (int i=0;i<levels;++i) {
        bind(bloom[i],false); gl.UseProgram(downProgram);
        useTexture(downProgram,"source",0,from.id);
        gl.Uniform2f(uniform(downProgram,"texel"),1.f/from.w,1.f/from.h);
        gl.Uniform1i(uniform(downProgram,"first"),i==0?1:0);
        screen(downProgram); from=bloom[i].texture;
    }
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_ONE,GL_ONE);
    for (int i=levels-2;i>=0;--i) {
        bind(bloom[i],false); gl.UseProgram(upProgram);
        useTexture(upProgram,"source",0,from.id);
        gl.Uniform2f(uniform(upProgram,"texel"),1.f/from.w,1.f/from.h);
        screen(upProgram); from=bloom[i].texture;
    }
    gl.Disable(GL_BLEND); bind(ldr,false); gl.UseProgram(presentProgram);
    useTexture(presentProgram,"world",0,lit.texture.id);
    useTexture(presentProgram,"bloom",1,bloom[0].texture.id);
    struct Grade { float shadow[3],highlight[3],saturation; };
    static constexpr Grade grades[]{
        {{.92f,1.00f,1.07f},{1.12f,1.03f,.90f},1.22f}, {{.88f,1.00f,1.05f},{1.00f,1.10f,1.07f},1.24f},
        {{1.03f,.92f,1.09f},{1.06f,1.10f,.94f},1.28f}, {{1.00f,.90f,.88f},{1.16f,1.02f,.86f},1.26f},
        {{.92f,.93f,1.12f},{1.06f,1.02f,1.12f},1.24f}};
    const auto& look=grades[static_cast<int>(g.stage->biome)];
    gl.Uniform1f(uniform(presentProgram,"strength"),graphics.bloom==0?0.f:graphics.bloom==1?.42f:graphics.bloom==2?.88f:1.12f);
    gl.Uniform1f(uniform(presentProgram,"exposure"),.86f);
    gl.Uniform1f(uniform(presentProgram,"time"),clockTime);
    gl.Uniform1f(uniform(presentProgram,"slow"),clamp((1-g.timeScale)/.72f));
    gl.Uniform1f(uniform(presentProgram,"hurt"),g.pain);
    gl.Uniform1f(uniform(presentProgram,"punch"),motionFx?g.punch:0.f);
    gl.Uniform1f(uniform(presentProgram,"flash"),g.flash);
    gl.Uniform1f(uniform(presentProgram,"combo"),clamp(g.combo/6.f)*(g.comboTimer>0?1.f:0.f));
    gl.Uniform1f(uniform(presentProgram,"saturation"),graphics.grading==0?1.f:graphics.grading==1?1.08f:look.saturation+(graphics.grading==3?.09f:0.f));
    gl.Uniform1f(uniform(presentProgram,"zoom"),cameraView.zoom);
    gl.Uniform2f(uniform(presentProgram,"focus"),cameraView.focus.x/width,1-cameraView.focus.y/height);
    const auto perspective=cameraView.perspective();
    gl.Uniform2f(uniform(presentProgram,"perspective"),perspective.x,perspective.y);
    gl.Uniform1f(uniform(presentProgram,"overscan"),cameraView.overscan());
    gl.Uniform2f(uniform(presentProgram,"roll"),std::cos(cameraView.roll()),std::sin(cameraView.roll()));
    const float grade=graphics.grading==0?0.f:graphics.grading==1?.5f:graphics.grading==2?1.f:1.3f;
    gl.Uniform3f(uniform(presentProgram,"shadowTint"),1+(look.shadow[0]-1)*grade,1+(look.shadow[1]-1)*grade,1+(look.shadow[2]-1)*grade);
    gl.Uniform3f(uniform(presentProgram,"highlightTint"),1+(look.highlight[0]-1)*grade,1+(look.highlight[1]-1)*grade,1+(look.highlight[2]-1)*grade);
    screen(presentProgram);
    output(); gl.UseProgram(fxaaProgram);
    useTexture(fxaaProgram,"image",0,ldr.texture.id);
    gl.Uniform2f(uniform(fxaaProgram,"texel"),1.f/ldr.texture.w,1.f/ldr.texture.h);
    gl.Uniform1f(uniform(fxaaProgram,"time"),clockTime);
    gl.Uniform1i(uniform(fxaaProgram,"quality"),graphics.antialias);
    screen(fxaaProgram);
}
} // namespace vl
