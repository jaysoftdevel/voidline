#include "render/renderer.hpp"

namespace vl {
void Renderer::drawLights() {
    const int size=graphics.lighting==0?240:graphics.lighting==1?360:graphics.lighting==2?480:640;
    if (lightMap.texture.w!=size) { destroy(lightMap); lightMap=target(size,size/2); }
    bind(lightMap,true);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_ONE,GL_ONE); gl.Enable(GL_SCISSOR_TEST);
    gl.UseProgram(lightProgram);
    const int w=lightMap.texture.w,h=lightMap.texture.h;
    const int count=std::min<int>(lights.size(),graphics.lighting==0?8:graphics.lighting==1?16:graphics.lighting==2?34:50);
    for (int i=0;i<count;++i) {
        const auto& l=lights[i];
        const float r=l.radius;
        const Rect bounds{l.p.x-r,l.p.y-r,r*2,r*2};
        if (!bounds.overlaps({0,0,width,height})) continue;
        const int x0=std::max(0,int(std::floor(bounds.x*w/width))), x1=std::min(w,int(std::ceil(bounds.right()*w/width)));
        const int y0=std::max(0,int(std::floor((height-bounds.bottom())*h/height))), y1=std::min(h,int(std::ceil((height-bounds.y)*h/height)));
        gl.Scissor(x0,y0,x1-x0,y1-y0);
        std::array<float,22*4> data{};
        int n=0;
        if (l.casts && graphics.shadows>0) for (const auto& b:blockers) {
            if (n>= (graphics.shadows==1?8:graphics.shadows==2?16:22) || !bounds.overlaps(b) || b.contains(l.p)) continue;
            data[n*4]=b.x; data[n*4+1]=b.y; data[n*4+2]=b.right(); data[n*4+3]=b.bottom(); ++n;
        }
        gl.Uniform1i(uniform(lightProgram,"blockers"),n);
        if (n) gl.Uniform4fv(uniform(lightProgram,"occluders[0]"),n,data.data());
        gl.Uniform1i(uniform(lightProgram,"soft"),graphics.shadows>=2?1:0);
        gl.Uniform4f(uniform(lightProgram,"lightData"),l.p.x,l.p.y,r,l.energy*(.75f+.125f*graphics.glow));
        gl.Uniform3f(uniform(lightProgram,"lightColor"),l.tint.r,l.tint.g,l.tint.b);
        gl.Uniform2f(uniform(lightProgram,"direction"),l.direction.x,l.direction.y);
        gl.Uniform1f(uniform(lightProgram,"cone"),l.cone);
        screen(lightProgram);
    }
    gl.Disable(GL_SCISSOR_TEST); gl.Disable(GL_BLEND);
}
} // namespace vl
