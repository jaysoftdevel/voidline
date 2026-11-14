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

// Jax Mercer: oversized breacher helmet with a square blast brow, dual filters and a solid amber visor.
void Renderer::drawMercerHead(const Humanoid& h,const Face& f) {
    const Vec c=f.head; const float d=f.ft,side=f.fa;
    ellipse(c+Vec{0,-.8f},{8.7f,8.f},h.ink);
    ellipse(c+Vec{0,-1.2f},{8.0f,7.3f},h.shell.energy(.93f));
    polygon(c+Vec{-7.7f,-4.3f},c+Vec{7.f,-4.3f},c+Vec{7.8f,-.5f},c+Vec{-7.8f,-.5f},h.leather);
    polygon(c+Vec{-7.f,-5.1f},c+Vec{4.9f,-6.3f},c+Vec{7.3f,-3.8f},c+Vec{-7.3f,-2.9f},h.shell.energy(1.26f));
    const Vec a=c+Vec{-5.8f,-.5f},b=c+Vec{d*(6.1f*side+(1-side)*5.8f),-.65f};
    line(a,b,4.8f,h.ink); line(a,b,2.85f,hex(0xfeb451).energy(2.3f));
    halo((a+b)*.5f,11,hex(0xffa03d).alpha(.26f));
    polygon(c+Vec{-4.6f,2.3f},c+Vec{5.8f,2.3f},c+Vec{4.7f,6.1f},c+Vec{-4.1f,6.1f},h.leather);
    for (int i=-1;i<=1;i+=2) {
        const Vec p=c+Vec{i*5.3f,4.3f};
        ellipse(p,{2.15f,2.5f},h.ink); ellipse(p,{1.45f,1.8f},h.shadow.energy(1.7f));
        line(p-Vec{1.f,0},p+Vec{1.f,0},.8f,h.trim.energy(1.7f));
    }
    // Reinforced squared crest and independently articulated ear protection.
    polygon(c+Vec{-3.f,-8.8f},c+Vec{2.2f,-8.8f},c+Vec{3.f,-6.2f},c+Vec{-3.f,-6.2f},h.trim.energy(.9f));
    ellipse(f.ear,{3.4f,4.1f},h.ink); ellipse(f.ear,{2.5f,3.1f},h.shell.energy(.8f));
}

// Darius Cole: aerodynamic falcon visor, pitched hood and a projecting recon sensor.
void Renderer::drawColeHead(const Humanoid& h,const Face& f) {
    const Vec c=f.head; const float d=f.ft;
    ellipse(c+Vec{0,-1.f},{7.0f,7.5f},h.ink);
    ellipse(c+Vec{0,-1.2f},{6.1f,6.6f},h.suit.energy(1.33f));
    triangle(c+Vec{-6.5f,-4.6f},c+Vec{d*3.f,-11.1f},c+Vec{d*10.1f,-4.9f},h.shell.energy(.98f));
    triangle(c+Vec{-5.8f,-3.9f},c+Vec{d*3.f,-9.f},c+Vec{d*8.8f,-3.5f},h.shadow.energy(1.8f));
    const Vec eye=c+Vec{d*2.5f,-.8f};
    triangle(eye+Vec{-d*5.f,-1.7f},eye+Vec{d*4.3f,-1.6f},eye+Vec{d*.3f,4.2f},h.ink);
    line(eye+Vec{-d*3.f,-1.f},eye+Vec{d*1.1f,3.f},2.f,hex(0x7df7ff).energy(2.1f));
    line(eye+Vec{d*1.1f,3.f},eye+Vec{d*3.3f,-1.1f},1.7f,hex(0x51d8f0).energy(2.1f));
    halo(eye,8,h.trim.alpha(.25f));
    polygon(c+Vec{-d*3.f,4.2f},c+Vec{d*5.f,4.2f},c+Vec{d*4.3f,7.1f},c+Vec{-d*2.7f,7.1f},h.leather);
    // Backward swept comms fin (unique aerodynamic profile).
    triangle(c+Vec{-d*4.4f,-5.9f},c+Vec{-d*14.4f,-10.7f},c+Vec{-d*7.1f,-2.4f},h.shell.energy(1.13f));
    ellipse(f.ear,{1.8f,2.5f},h.trim.energy(1.2f));
}

// Sera Nyx: angular ivory-and-violet sealed helmet and floating holographic sensor blades.
void Renderer::drawNyxHead(const Humanoid& h,const Face& f) {
    const Vec c=f.head; const float d=f.ft;
    ellipse(c+Vec{0,-.9f},{7.f,7.9f},h.ink);
    ellipse(c+Vec{0,-1.2f},{6.1f,7.1f},hex(0xd7d3d8));
    triangle(c+Vec{-d*4.8f,-5.8f},c+Vec{d*7.2f,-5.1f},c+Vec{d*5.f,5.9f},h.shadow.energy(1.8f));
    triangle(c+Vec{-d*3.f,-3.9f},c+Vec{d*5.f,-3.5f},c+Vec{d*2.9f,3.6f},hex(0x8f42ff).energy(1.7f));
    line(c+Vec{d*3.f,-3.9f},c+Vec{d*2.2f,2.5f},1.1f,hex(0xf3c9ff).energy(2.1f));
    halo(c+Vec{d*2.f,-1.f},10,h.visor.alpha(.32f));
    polygon(c+Vec{-d*3.7f,3.8f},c+Vec{d*3.2f,5.f},c+Vec{d*3.1f,7.8f},c+Vec{-d*4.f,6.2f},h.shell.energy(1.3f));
    // Split holographic crown, placed above/behind the head, clearly unlike the other helmets.
    for (int i=0;i<3;++i) {
        const float angle=-2.5f+i*.31f;
        const Vec a=c+Vec{std::cos(angle)*11.2f,std::sin(angle)*10.f};
        const Vec b=c+Vec{std::cos(angle-.19f)*12.3f,std::sin(angle-.19f)*10.7f};
        line(a,b,1.1f,h.trim.energy(2.1f).alpha(.82f));
    }
    ellipse(f.ear,{2.5f,3.3f},h.ink); ellipse(f.ear,{1.8f,2.5f},hex(0xdbd5db));
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
