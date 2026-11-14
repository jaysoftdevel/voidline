#include "render/renderer.hpp"

namespace vl {
void Renderer::water(const Game& g,bool high) {
    flush();
    gl.Disable(GL_BLEND); gl.UseProgram(waterProgram);
    useTexture(waterProgram,"reflection",0,mirror.texture.id);
    constexpr float levels[]{535,523,533,531,532};
    constexpr float reflect[]{.37f,.76f,.47f,.53f,.42f};
    const Color tint=hex(g.index==3?g.stage->accent:g.stage->cold);
    gl.Uniform1f(uniform(waterProgram,"surface"),levels[g.index]+offset.y);
    gl.Uniform1f(uniform(waterProgram,"time"),g.time);
    gl.Uniform1f(uniform(waterProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(waterProgram,"reflectivity"),reflect[g.index]*(high?1.f:.76f));
    gl.Uniform3f(uniform(waterProgram,"tint"),tint.r,tint.g,tint.b);
    screen(waterProgram);
    gl.Enable(GL_BLEND);
}
} // namespace vl
