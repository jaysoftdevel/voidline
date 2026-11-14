#include "render/renderer.hpp"

namespace vl {
void Renderer::outlinedText(Vec p,std::string_view value,float size,Color color,Color ink) {
    const float w=std::max(1.4f,size*.075f);
    for (const Vec o:{Vec{-w,0},Vec{w,0},Vec{0,-w},Vec{0,w},Vec{-w,-w},Vec{w,-w},Vec{-w,w},Vec{w,w}})
        text(p+o,value,size,ink.alpha(color.a),true);
    text(p,value,size,color,true);
}

float Renderer::measure(std::string_view value,float size,bool bold) const {
    const auto& font=fonts[bold?1:0];
    float total=0;
    for (unsigned char c:value) if (c<127 && c>=32) total+=font.advances[c-32];
    return total*(size/48);
}

void Renderer::text(Vec p,std::string_view value,float size,Color color,bool bold) {
    const auto& font=fonts[bold?1:0];
    const float scale=size/48;
    for (unsigned char c:value) {
        if (c<32 || c>=127) continue;
        const int glyph=c-32;
        if (c!=' ') quad({p.x-3*scale,p.y-49*scale,54*scale,65*scale},color,color,font.texture.id,
                         {float(glyph%16)/16,float(glyph/16)/6,1.f/16,1.f/6});
        p.x+=font.advances[glyph]*scale;
    }
}
} // namespace vl
