#include "render/figures/humanoid.hpp"

namespace vl {
// Mara Voss: cropped hair, orange goggles on a headband, an ear cup and a scar.
void Renderer::drawVossHead(const Humanoid& h,const Face& f) {
    const Color ink=h.ink;
    const Vec head=f.head;
    const float ft=f.ft,fa=f.fa;
    ellipse(head+Vec{-ft*1.2f*fa,-3.1f},{6.6f,4.3f},ink);
    ellipse(head+Vec{-ft*1.2f*fa,-3.2f},{5.9f,3.7f},h.hair);
    for (int k=0;k<4;++k) {
        const Vec b=head+Vec{-ft*(1.f+k*1.5f)*fa+(1-fa)*(k-1.5f)*1.6f,-6.4f};
        triangle(b-Vec{1.2f,0},b+Vec{1.2f,0},b+Vec{ft*.8f,-2.3f},h.hair);
    }
    line(head+Vec{-6.4f,-3.4f},head+Vec{6.4f,-3.4f},2.f,ink);
    line(head+Vec{-6.2f,-3.4f},head+Vec{6.2f,-3.4f},1.2f,h.leather.energy(1.3f));
    const Vec lens=head+Vec{ft*2.8f*fa,-3.9f};
    ellipse(lens,{3.2f+(1-fa)*1.2f,2.1f},ink);
    ellipse(lens,{2.5f+(1-fa)*1.2f,1.4f},h.trim.energy(2.3f));
    halo(lens,9,h.trim.alpha(.22f));
    ellipse(f.ear,{2.1f,2.6f},ink); ellipse(f.ear,{1.5f,2.f},h.shadow.energy(1.5f));
    line(f.ear+Vec{-.8f,-.8f},f.ear+Vec{.8f,-.8f},.7f,h.trim.energy(2.f));
    line(f.ear+Vec{-.8f,.6f},f.ear+Vec{.8f,.6f},.7f,h.trim.energy(2.f));
    if (fa>.4f) line(Vec{head.x+ft*3.8f*fa,head.y+1.2f},Vec{head.x+ft*4.7f*fa,head.y+3.4f},.9f,hex(0xb85a50).alpha(.85f));
}

// Iona Rook: curly hair, a cyan monocle and an ear cup.
void Renderer::drawRookHead(const Humanoid& h,const Face& f) {
    const Color ink=h.ink;
    const Vec head=f.head;
    for (int k=0;k<5;++k) {
        const float a=-pi*.92f+k*pi*.21f;
        const Vec c=head+Vec{std::cos(a)*5.6f*(1-.25f*f.fa)+f.ft*1.2f*f.fa,std::sin(a)*5.4f-1.2f};
        ellipse(c,{2.5f,2.4f},ink);
        ellipse(c,{2.f,1.9f},h.hair);
    }
    ellipse(f.eyeA,{2.7f,2.4f},ink);
    ellipse(f.eyeA,{2.1f,1.8f},h.trim.energy(1.7f).alpha(.82f));
    line(f.eyeA+Vec{-2.f*f.ft,-.4f},f.ear+Vec{0,-1.2f},.8f,h.edge.alpha(.8f));
    halo(f.eyeA,8,h.trim.alpha(.25f));
    ellipse(f.ear,{2.f,2.5f},ink); ellipse(f.ear,{1.4f,1.9f},h.shadow.energy(1.4f));
}

// Kade Vale: swept grey hair, a beard, goggles on the forehead and a headset with a microphone.
void Renderer::drawValeHead(const Humanoid& h,const Face& f) {
    const Color ink=h.ink;
    const Vec head=f.head;
    const float ft=f.ft,fa=f.fa;
    ellipse(head+Vec{-ft*1.6f*fa,-3.3f},{6.6f,4.2f},ink);
    ellipse(head+Vec{-ft*1.6f*fa,-3.4f},{5.9f,3.6f},h.hair);
    for (int k=0;k<4;++k) line(head+Vec{-ft*(4.8f-k*1.5f)*fa,-6.3f+k*.4f},head+Vec{ft*(1.2f+k*.8f)*fa,-5.2f+k*.8f},.8f,h.hair.energy(1.4f));
    ellipse(head+Vec{ft*2.f*fa,4.2f},{5.1f,3.7f},ink);
    ellipse(head+Vec{ft*2.f*fa,4.1f},{4.5f,3.1f},hex(0x9aa0aa));
    line(Vec{head.x+ft*2.2f*fa,head.y+3.1f},Vec{head.x+ft*5.4f*fa,head.y+3.1f},1.1f,hex(0x70757f));
    line(head+Vec{-6.4f,-4.4f},head+Vec{6.4f,-4.4f},2.2f,ink);
    line(head+Vec{-6.2f,-4.4f},head+Vec{6.2f,-4.4f},1.3f,h.leather.energy(1.2f));
    const Vec lens=head+Vec{ft*2.4f*fa,-4.9f};
    ellipse(lens,{3.4f+(1-fa)*1.2f,2.3f},ink);
    ellipse(lens,{2.7f+(1-fa)*1.2f,1.7f},hex(0x8a78e0).energy(1.5f));
    line(lens+Vec{-1.4f,-.6f},lens+Vec{.6f,-.9f},.7f,hex(0xffffff).alpha(.7f));
    ellipse(f.ear,{2.4f,2.9f},ink); ellipse(f.ear,{1.8f,2.3f},h.shadow.energy(1.5f));
    ellipse(f.ear+Vec{0,-.6f},{.8f,.8f},h.trim.energy(2.5f));
    const Vec m0=f.ear+Vec{ft*1.f,1.6f}, m1=Vec{head.x+ft*2.5f*fa,head.y+5.3f}, m2=Vec{head.x+ft*5.2f*fa,head.y+4.5f};
    line(m0,m1,.9f,ink); line(m1,m2,.9f,ink);
    ellipse(m2,{1.1f,1.1f},h.trim.energy(1.6f));
}

// Soldiers wear a closed helmet with a glowing visor and a chin guard. The visor dies out when the soldier falls.
void Renderer::drawHelmet(const Humanoid& h,const Face& f) {
    const Color ink=h.ink;
    const Vec head=f.head;
    const float ft=f.ft,fa=f.fa;
    ellipse(head+Vec{0,-1.2f},{7.4f,6.7f},ink);
    ellipse(head+Vec{0,-1.3f},{6.6f,5.9f},h.shell.energy(1.05f));
    polygon(head+Vec{-6.2f,-.6f},head+Vec{6.2f,-.6f},head+Vec{5.6f,2.6f},head+Vec{-5.6f,2.6f},h.suit.energy(.9f));
    const Vec visorA=Vec{head.x+ft*(-1.2f)*fa-(1-fa)*4.6f,head.y-.4f}, visorB=Vec{head.x+ft*6.4f*fa+(1-fa)*4.6f,head.y-.4f};
    const float glow=h.fall>.4f?(std::sin(h.time*38.f)>.3f?.35f:0.f):1.f;
    line(visorA,visorB,3.6f,ink);
    line(visorA,visorB,2.3f,h.visor.energy(2.4f*glow));
    if (glow>0) halo((visorA+visorB)*.5f,10,h.visor.alpha(.32f*glow));
    line(head+Vec{-4.8f,-5.f},head+Vec{3.4f,-6.2f},1.1f,h.edge.alpha(.85f));
    line(head+Vec{ft*2.f*fa,-6.4f},head+Vec{ft*2.f*fa,-.4f},.9f,ink.alpha(.6f));
    polygon(head+Vec{-2.f*ft,3.4f},head+Vec{ft*5.2f*fa,3.4f},head+Vec{ft*4.6f*fa,6.4f},head+Vec{-ft*1.4f,6.6f},h.shell.energy(.8f));
    line(head+Vec{-ft*2.2f,3.6f},head+Vec{ft*4.8f*fa,3.6f},.8f,ink);
    ellipse(f.ear,{2.1f,2.6f},ink); ellipse(f.ear,{1.5f,2.f},h.shadow.energy(1.5f));
}
} // namespace vl
