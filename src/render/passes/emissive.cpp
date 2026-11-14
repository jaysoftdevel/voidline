#include "render/renderer.hpp"

namespace vl {
void Renderer::emissive(Vec p,float r,Color c,float e) {
    halo(p,r*5.5f,c.energy(e*1.13f).alpha(.36f));
    ellipse(p,{r,r*.83f},c.energy(e));
}

void Renderer::limb(Vec a,Vec b,float thickness,Color plate,Color rim) {
    const Vec d=normal(b-a), edge{d.y,-d.x};
    line(a,b,thickness+3.4f,hex(0x06080d));
    line(a,b,thickness,plate);
    line(a+edge*thickness*.29f,b+edge*thickness*.29f,std::max(1.f,thickness*.21f),rim.alpha(.63f));
    ellipse(a,{thickness*.42f,thickness*.42f},plate.energy(.83f));
}
} // namespace vl
