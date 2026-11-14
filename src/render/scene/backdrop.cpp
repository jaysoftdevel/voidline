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
    for (int i=0;i<12;++i) {
        const float x=std::fmod(260+i*131.3f+offset.x*.072f,1440.f);
        const float y=123+hash(i+60)*255;
        const float pulse=.47f+.24f*std::sin(g.time*1.7f+i*1.32f);
        halo({x+tilt.x*15,y+tilt.y*6},19+hash(i)*15,haze.alpha(pulse*.35f));
    }
}
} // namespace vl
