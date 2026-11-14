#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::backdrop(const Game& g) {
    const float travel=clamp(-offset.x/(g.stage->worldWidth-width));
    const Vec tilt=cameraView.tilt;
    cover(g.stage->art,{0,0,width,height},1.12f,{mix(.24f,.76f,travel)-tilt.x*.16f,.51f-tilt.y*.10f},hex(0xc6d3e4));
    gradient({0,0,width,height},hex(0x091420).alpha(.12f),hex(0x071019).alpha(.65f));
    const Color haze=hex(g.stage->cold);
    for (int i=0;i<18;++i) {
        const float x=std::fmod(260+i*131.3f+offset.x*.072f,1440.f);
        const float y=123+hash(i+60)*255;
        const float pulse=.47f+.24f*std::sin(g.time*1.7f+i*1.32f);
        halo({x+tilt.x*15,y+tilt.y*6},19+hash(i)*15,haze.alpha(pulse*.35f));
    }
    for (int i=0;i<9;++i) {
        const float depth=.18f+.07f*i;
        const float x=std::fmod(90+i*182.0f+offset.x*(.16f+depth*.07f),1540.f)-70.f;
        const float h=140+hash(i+91)*220;
        const float y=height-(110+hash(i+45)*40);
        rect({x+tilt.x*22*depth,y-h,56+hash(i+17)*52,h},hex(0x0c1622).alpha(.18f+.05f*depth));
        line({x+28,y-h},{x+28,y},1.2f,hex(0x5fcfff).alpha(.06f+.04f*depth));
        if (i%2==0) halo({x+28+tilt.x*6,y-h+25+hash(i+7)*30},16+hash(i+4)*12,haze.alpha(.12f+.07f*depth));
    }
    for (int i=0;i<7;++i) {
        const float x=std::fmod(240+i*211.0f+offset.x*.11f,1520.f)-90.f;
        const float y=155+hash(i+130)*170;
        line({x+tilt.x*10,y},{x+86+tilt.x*14,y+18+hash(i+8)*16},1.0f,hex(0xe0f4ff).alpha(.04f));
    }
}
} // namespace vl
