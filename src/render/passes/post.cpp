#include "render/renderer.hpp"

namespace vl {
void Renderer::post(const Game& g,bool high,bool motionFx) {
    bind(lit,false);
    gl.UseProgram(atmosphereProgram);
    useTexture(atmosphereProgram,"world",0,raw.texture.id);
    useTexture(atmosphereProgram,"lighting",1,lightMap.texture.id);
    const Color tint=hex(g.stage->cold);
    gl.Uniform3f(uniform(atmosphereProgram,"haze"),tint.r,tint.g,tint.b);
    gl.Uniform1f(uniform(atmosphereProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(atmosphereProgram,"time"),g.time);
    gl.Uniform1f(uniform(atmosphereProgram,"fogStrength"),g.stage->biome==Biome::water || g.stage->biome==Biome::biodome?.72f:.55f);
    gl.Uniform2f(uniform(atmosphereProgram,"lightTexel"),1.f/lightMap.texture.w,1.f/lightMap.texture.h);
    gl.Uniform2f(uniform(atmosphereProgram,"worldTexel"),1.f/raw.texture.w,1.f/raw.texture.h);
    gl.Uniform1f(uniform(atmosphereProgram,"relief"),high?7.5f:4.5f);
    screen(atmosphereProgram);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    actors(g);
    flush();
    gl.Disable(GL_BLEND);

    const int levels=high?4:2;
    Texture from=lit.texture;
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
    const auto& p=g.player.box;
    gl.Uniform1f(uniform(presentProgram,"strength"),high?.89f:.64f);
    gl.Uniform1f(uniform(presentProgram,"exposure"),.82f);
    gl.Uniform1f(uniform(presentProgram,"time"),clockTime);
    gl.Uniform1f(uniform(presentProgram,"slow"),clamp((1-g.timeScale)/.72f));
    gl.Uniform1f(uniform(presentProgram,"hurt"),g.pain);
    gl.Uniform1f(uniform(presentProgram,"punch"),motionFx?g.punch:0.f);
    gl.Uniform1f(uniform(presentProgram,"flash"),g.flash);
    gl.Uniform1f(uniform(presentProgram,"combo"),clamp(g.combo/6.f)*(g.comboTimer>0?1.f:0.f));
    gl.Uniform1f(uniform(presentProgram,"saturation"),look.saturation);
    gl.Uniform2f(uniform(presentProgram,"focus"),clamp((p.center().x+offset.x)/width,.2f,.8f),
                 1-clamp((p.center().y+offset.y)/height,.2f,.8f));
    gl.Uniform3f(uniform(presentProgram,"shadowTint"),look.shadow[0],look.shadow[1],look.shadow[2]);
    gl.Uniform3f(uniform(presentProgram,"highlightTint"),look.highlight[0],look.highlight[1],look.highlight[2]);
    screen(presentProgram);
    output(); gl.UseProgram(fxaaProgram);
    useTexture(fxaaProgram,"image",0,ldr.texture.id);
    gl.Uniform2f(uniform(fxaaProgram,"texel"),1.f/ldr.texture.w,1.f/ldr.texture.h);
    gl.Uniform1f(uniform(fxaaProgram,"time"),clockTime);
    screen(fxaaProgram);
}
} // namespace vl
