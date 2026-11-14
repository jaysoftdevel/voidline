#include "render/renderer.hpp"

namespace vl {
void Renderer::water(const Game& g,bool high) {
    flush();
    gl.Disable(GL_BLEND); gl.UseProgram(waterProgram);
    useTexture(waterProgram,"reflection",0,mirror.texture.id);
    constexpr float levels[]{535,523,533,531,532};
    constexpr float reflect[]{.37f,.76f,.47f,.53f,.42f};
    const Color tint=hex(g.stage->biome==Biome::forge?g.stage->accent:g.stage->cold);
    gl.Uniform1f(uniform(waterProgram,"surface"),levels[static_cast<int>(g.stage->biome)]+offset.y);
    gl.Uniform1f(uniform(waterProgram,"time"),g.time);
    gl.Uniform1f(uniform(waterProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(waterProgram,"reflectivity"),reflect[static_cast<int>(g.stage->biome)]*(high?1.f:.76f));
    gl.Uniform3f(uniform(waterProgram,"tint"),tint.r,tint.g,tint.b);
    screen(waterProgram);
    gl.Enable(GL_BLEND);
}
} // namespace vl
