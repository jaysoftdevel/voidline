#include "render/figures/humanoid.hpp"

namespace vl {

// Neck, skull, ear and the face. Eyes, nose and mouth are only drawn for bare heads. The characteristic
// headgear of each look is drawn by its own function afterwards.
Renderer::Face Renderer::drawHead(const Humanoid& h,const Rig& g) {
    using Look=Humanoid::Look;
    const Color ink=h.ink;
    const float tf=g.tf,fa=g.fa;
    Face face;
    face.head=g.head; face.fa=fa; face.ft=tf<0?-1.f:1.f; face.look=g.look;
    const Vec head=g.head;
    const float ft=face.ft;
    line(g.neck+Vec{0,-1.f},head+Vec{0,4.f},4.6f,ink);
    line(g.neck+Vec{0,-1.f},head+Vec{0,4.f},3.1f,h.skin.energy(.8f));
    if (h.look==Look::rook) {
        // Curls behind the head.
        for (int pass=0;pass<2;++pass)
            for (int k=0;k<9;++k) {
                const float a=pi+k*pi/8.f;
                const Vec c=head+Vec{std::cos(a)*6.4f-tf*1.4f*fa,std::sin(a)*6.f-.8f};
                if (pass==0) ellipse(c,{3.1f,3.1f},ink);
                else ellipse(c,{2.5f,2.5f},mix(h.hair,hex(0x2a2a3a),(k%3)*.18f));
            }
    }
    ellipse(head,{6.7f,7.f},ink);
    ellipse(head,{5.9f,6.3f},h.skin);
    ellipse(head+Vec{tf*1.6f*fa,3.2f},{4.4f,3.2f},h.skin.energy(.9f));
    face.ear=Vec{head.x+tf*(-2.4f)*fa,head.y+.9f}+Vec{(1-fa)*(-5.2f),0};
    ellipse(face.ear,{1.5f,2.f},h.skin.energy(.85f));
    face.eyeA=Vec{head.x+ft*3.f*fa+(1-fa)*2.3f,head.y-.3f-g.look*.9f};
    face.eyeB=Vec{head.x+ft*3.f*fa-(1-fa)*2.3f,head.y-.3f-g.look*.9f};
    if (h.look!=Look::soldier) {
        // A collapsed figure has closed eyes.
        auto eye=[&](Vec p) {
            if (h.fall>.55f) { line(p+Vec{-1.6f,0},p+Vec{1.6f,.4f},1.1f,ink); return; }
            ellipse(p,{1.7f,1.25f},hex(0xf1f1ee));
            ellipse(p+Vec{ft*.55f*fa,0},{.85f,.95f},hex(0x1a1511));
            line(p+Vec{-2.1f,-2.1f},p+Vec{2.1f,-1.8f},.9f,h.hair.energy(.9f));
        };
        eye(face.eyeA);
        if (fa<.72f) eye(face.eyeB);
        if (fa>.35f) polygon(Vec{head.x+ft*5.4f*fa,head.y-.4f},Vec{head.x+ft*7.7f*fa,head.y+1.8f},Vec{head.x+ft*5.2f*fa,head.y+2.3f},Vec{head.x+ft*4.6f*fa,head.y+.4f},h.skin.energy(.72f));
        else line(Vec{head.x,head.y+.4f},Vec{head.x,head.y+2.2f},1.f,h.skin.energy(.65f));
        // The mouth opens in a grimace when the figure is hit.
        const float open=std::max(h.flinch,h.fall)*1.8f;
        line(Vec{head.x+ft*2.2f*fa-(1-fa)*1.4f,head.y+3.9f},Vec{head.x+ft*4.6f*fa+(1-fa)*1.4f,head.y+3.9f+open*.4f},.9f+open*.35f,ink.alpha(.7f));
    }
    switch (h.look) {
    case Look::voss: drawVossHead(h,face); break;
    case Look::rook: drawRookHead(h,face); break;
    case Look::vale: drawValeHead(h,face); break;
    case Look::soldier: drawHelmet(h,face); break;
    }
    return face;
}
} // namespace vl
