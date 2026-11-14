#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"
#include "render/figures/humanoid.hpp"

using namespace vl::figures;

namespace vl {
void Renderer::polygon(Vec a,Vec b,Vec c,Vec d,Color color) {
    triangle(a,b,c,color); triangle(a,c,d,color);
}

void Renderer::armor(Vec a,Vec b,float top,float bottom,Color tint,Color edge,bool near) {
    const Vec d=normal(b-a), n{d.y,-d.x};
    const Vec left=a-n*top, right=a+n*top;
    const Vec endLeft=b-n*bottom,endRight=b+n*bottom;
    const Color ink=hex(0x06080d);
    line(a,b,2*std::max(top,bottom)+3.4f,ink);
    triangle(left,right,endRight,tint.energy(.69f));
    triangle(left,endRight,endLeft,tint.energy(.89f));
    const Vec ribA=a+n*top*.28f,ribB=b+n*bottom*.31f;
    triangle(ribA,right,endRight,tint.energy(near?1.11f:.86f));
    triangle(ribA,endRight,ribB,tint.energy(near?1.03f:.82f));
    line(right,endRight,near?1.02f:.75f,edge.alpha(near?.88f:.46f));
    line(left,endLeft,.75f,tint.energy(.49f));
    if (near) {
        line(mix(left,endLeft,.17f),mix(right,endRight,.17f),.77f,ink.alpha(.76f));
        line(mix(left,endLeft,.71f),mix(right,endRight,.71f),.72f,ink.alpha(.67f));
    }
}
} // namespace vl
