#include "renderer.hpp"
#include <functional>

namespace vl {
namespace {
struct GunArt { const char* name; float length,aspect,muzzleV,gripU,gripV,foreU,foreV; };
constexpr GunArt gunArt[]{
    {"carbine",58,.353f,.39f,.29f,.68f,.70f,.56f},
    {"shotgun",62,.314f,.36f,.27f,.76f,.70f,.52f},
    {"arc",70,.231f,.39f,.30f,.80f,.65f,.55f}
};
}

// A side-view human rig. The player and the soldier enemies share it. All positions are in screen space.
struct Renderer::Humanoid {
    enum class Look { voss, rook, vale, soldier };
    Look look=Look::soldier;
    Vec ground;
    int facing=1,stride=1;
    float turn=1;
    std::array<Vec,2> heels{};
    std::array<float,2> footAngle{};
    bool grounded=true;
    float lean=0,crouch=0,run=0,weight=0,cycle=0,compression=0,pelvis=0,tuck=0;
    float time=0,flinch=0,landed=0,recoil=0,spin=0;
    Vec velocity,aim{1,0},nearHand,farHand;
    bool farFree=false,gripFromChest=false,scarf=true;
    float gripNear=9,gripFar=19;
    Color suit,shell,trim,edge,shadow,ink,skin,hair,visor,leather,glove;
    std::function<void(Vec,Vec)> weapon;
};

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

void Renderer::humanoid(const Humanoid& h) {
    using Look=Humanoid::Look;
    const Color ink=h.ink;
    const float tf=h.turn, fa=std::abs(tf);
    const int dir=h.stride;
    const float counter=std::sin(h.cycle*2*pi)*h.weight;
    const float idle=1-h.weight;
    const float breathe=std::sin(h.time*2.3f)*(.25f+idle*.30f+h.run*.30f);
    const float bend=std::min(8.f,h.compression);
    const float squash=h.landed*2.4f;

    Vec hip{h.ground.x+h.lean*(9+h.crouch*5)-h.flinch*2.5f*tf,h.ground.y-33.2f+h.run*2.2f+h.pelvis+bend+h.crouch*17.f};
    for (const Vec& heel:h.heels) {
        const float x=std::min(31.f,std::abs(hip.x-heel.x));
        hip.y=std::max(hip.y,heel.y-4.5f-std::sqrt(29.9f*29.9f-x*x));
    }
    if (h.grounded) {
        halo(h.ground-Vec{0,1},24,hex(0x090f19).alpha(.50f));
        ellipse(h.ground,{20.f+h.crouch*4,2.8f},hex(0x090f16).alpha(.56f));
    }
    if (h.spin!=0) beginSpin({h.ground.x,h.ground.y-26+h.crouch*8},h.spin);

    // Legs. The far leg is drawn first and darker.
    auto leg=[&](int i,bool far) {
        const Vec contact=h.heels[i];
        const Vec toeRay=ray(h.footAngle[i]);
        const Vec forward{toeRay.x*dir,-toeRay.y};
        const float lift=h.grounded?std::max(0.f,std::max(contact.y,(contact+forward*8).y)-h.ground.y):0.f;
        const Vec heel=contact-Vec{0,2.4f+lift};
        const Vec toe=contact+forward*8-Vec{0,2.4f+lift};
        const Vec ankle=heel-Vec{0,2.1f};
        const Vec socket=hip+Vec{i==0?-2.f:2.f,i==0?-1.f:.5f};
        const Vec knee=solveKnee(socket,ankle,15.6f,14.6f,dir);
        const Color cloth=far?h.shadow:h.suit;
        const Color plate=far?h.shadow.energy(1.4f):h.shell;
        const Color boot=far?h.leather.energy(.6f):h.leather;
        // Boot: outline, upper, rubber sole with tread, toe cap and heel block.
        line(heel-Vec{2.7f*dir,0},toe+Vec{1.4f*dir,0},far?5.2f:6.2f,ink);
        line(heel-Vec{2.1f*dir,.5f},toe,far?3.4f:4.4f,boot);
        line(heel-Vec{2.4f*dir,2.f},toe+Vec{.9f*dir,2.f},1.5f,hex(far?0x14171d:0x262b34));
        if (!far) {
            for (int t=0;t<3;++t) {
                const Vec p=mix(heel,toe,.2f+t*.3f)+Vec{0,2.4f};
                line(p,p+Vec{0,.9f},.8f,ink);
            }
            line(heel-Vec{1.6f*dir,-.6f},toe-Vec{0,.9f},.8f,plate.alpha(.7f));
        }
        ellipse(toe-Vec{.8f*dir,.1f},{far?2.3f:2.8f,far?2.2f:2.6f},plate.energy(far?.8f:1.f));
        polygon(heel-Vec{2.8f*dir,1.f},heel-Vec{.4f*dir,1.f},heel-Vec{.4f*dir,-2.6f},heel-Vec{2.8f*dir,-2.6f},plate.energy(.75f));
        // Thigh, shin, greave and the plate over the thigh.
        armor(socket,knee,far?3.3f:4.5f,far?2.4f:3.3f,cloth,h.edge,!far);
        armor(knee,ankle,far?2.6f:3.4f,far?1.9f:2.5f,cloth.energy(.92f),h.edge,!far);
        armor(mix(knee,ankle,.2f),mix(knee,ankle,.86f),far?1.8f:2.1f,far?1.5f:1.7f,plate,h.edge,!far);
        ellipse(ankle,{far?2.6f:3.3f,2.2f},cloth.energy(.8f));
        if (!far) {
            armor(mix(socket,knee,.28f),mix(socket,knee,.66f),2.5f,2.4f,h.look==Look::soldier?plate:h.trim.energy(.72f),h.edge,true);
            const Vec strap=mix(socket,knee,.5f);
            line(strap-Vec{3.6f,0},strap+Vec{3.6f,0},1.1f,ink.alpha(.8f));
            ellipse(mix(socket,knee,.46f),{.8f,.8f},h.edge);
            // Laces across the instep.
            for (int t=0;t<2;++t) {
                const Vec p=mix(ankle,toe,.42f+t*.22f)-Vec{0,1.2f};
                line(p-Vec{1.1f,0},p+Vec{1.1f,0},.8f,h.trim.alpha(.9f));
            }
        }
        // Knee pad.
        ellipse(knee,{far?2.7f:3.5f,far?2.9f:3.8f},ink);
        ellipse(knee,{far?2.0f:2.7f,far?2.2f:3.0f},plate.energy(far?.9f:1.1f));
        if (!far) {
            line(knee+Vec{-2.2f,-1.2f},knee+Vec{2.f,-1.6f},.9f,h.edge.alpha(.85f));
            line(knee+Vec{-1.9f,.9f},knee+Vec{2.1f,1.1f},1.2f,h.trim.energy(1.15f));
        }
    };
    const int rear=h.heels[0].x*dir<h.heels[1].x*dir?0:1;
    leg(rear,true); leg(1-rear,false);

    // Spine. The neck follows the pelvis, leans with speed and sinks when the body compresses on landing.
    const float tilt=h.lean*(29-4*h.crouch);
    const Vec neck=hip+Vec{tilt+tf*(2.4f+h.crouch*5.8f)-h.flinch*3.f*tf+counter*.8f*tf,
                           -15.8f+h.crouch*5.5f+h.tuck*.65f-breathe*.5f+squash+h.flinch*.8f};
    const float spine=std::max(8.f,length(neck-hip));
    const Vec up=normal(neck-hip), fwdAxis{-up.y,up.x};
    auto at=[&](float u,float v) { return hip+up*u+fwdAxis*(v*tf); };
    const Vec waist=mix(hip,neck,.5f)+fwdAxis*(tf*(1.1f+h.crouch*1.8f));

    if (h.scarf) {
        const Vec base=neck+Vec{-2.5f*tf,2.f};
        const Vec wind{-clamp(h.velocity.x/420,-1.3f,1.3f)*4.6f-tf*2.4f,-clamp(h.velocity.y/760,-1.f,1.f)*3.2f};
        auto knot=[&](int i) {
            return base+Vec{wind.x*i*1.1f,i*1.05f+wind.y*i*.8f+std::sin(h.time*11-i*.85f)*i*.38f};
        };
        for (int pass=0;pass<2;++pass) {
            Vec from=base;
            for (int i=1;i<=8;++i) {
                const Vec to=knot(i);
                const float w=std::max(1.4f,5.8f-i*.55f);
                if (pass==0) line(from,to,w+2.6f,ink);
                else line(from,to,w,h.trim.energy(i%2?1.f:.8f));
                from=to;
            }
        }
    }

    // Backpack with character-specific gear.
    {
        const Vec top=at(spine*.92f,-8.4f), bottom=at(spine*.16f,-7.6f);
        armor(top,bottom,4.9f,4.4f,h.shadow.energy(1.15f),h.edge.energy(.68f),true);
        line(at(spine*.72f,-8.4f)+Vec{-3.f,0},at(spine*.72f,-8.4f)+Vec{3.f,0},1.1f,ink.alpha(.8f));
        const Vec antenna=at(spine*.98f,-9.5f);
        switch (h.look) {
        case Look::voss:
            armor(at(spine*.5f,-11.4f),at(spine*.3f,-11.4f),1.9f,1.9f,h.trim.energy(.7f),h.edge,true);
            line(antenna,antenna+Vec{-1.5f*tf,-15.f},1.1f,ink);
            ellipse(antenna+Vec{-1.5f*tf,-15.5f},{1.3f,1.3f},h.trim.energy(2.4f));
            break;
        case Look::rook:
            line(at(spine*1.02f,-8.6f)-Vec{5.5f,0},at(spine*1.02f,-8.6f)+Vec{5.5f,0},4.2f,ink);
            line(at(spine*1.02f,-8.6f)-Vec{5.f,0},at(spine*1.02f,-8.6f)+Vec{5.f,0},2.8f,h.leather.energy(1.4f));
            line(antenna+Vec{3.f,0},antenna+Vec{4.f*tf+3.f,-17.f},1.1f,ink);
            ellipse(antenna+Vec{4.f*tf+3.f,-17.5f},{1.4f,1.4f},h.trim.energy(2.6f));
            break;
        case Look::vale:
            armor(at(spine*.94f,-11.6f),at(spine*.2f,-11.6f),2.2f,2.2f,h.shadow.energy(1.5f),h.edge,true);
            for (int t=0;t<3;++t) ellipse(at(spine*(.78f-t*.14f),-11.9f),{1.1f,1.1f},h.trim.energy(2.5f));
            line(at(spine*.9f,-8.f),at(spine*1.16f,-1.f),1.8f,ink);
            line(at(spine*.9f,-8.f),at(spine*1.16f,-1.f),.9f,h.trim.energy(.9f));
            break;
        case Look::soldier:
            line(antenna,antenna+Vec{-2.f*tf,-12.f},1.1f,ink);
            ellipse(antenna+Vec{-2.f*tf,-12.5f},{1.1f,1.1f},h.visor.energy(2.2f));
            break;
        }
    }

    // Torso: abdomen, chest, plates, belt, pouches and straps.
    armor(waist,hip,5.9f,5.4f,h.suit.energy(.92f),h.edge,true);
    armor(neck,waist,7.1f+breathe*.5f,6.1f,h.suit,h.edge,true);
    {
        const float wide=(1-fa)*4.5f;
        const Vec p0=at(spine*.97f,-2.f)-Vec{wide,0}, p1=at(spine*.92f,6.5f)+Vec{wide,0};
        const Vec p2=at(spine*.44f,6.f)+Vec{wide,0}, p3=at(spine*.42f,-2.f)-Vec{wide,0};
        polygon(p0,p1,p2,p3,h.shell.energy(1.04f));
        polygon(mix(p0,p3,.55f),mix(p1,p2,.55f),p2,p3,h.shell.energy(.78f));
        line(p0,p1,1.1f,h.edge.alpha(.8f));
        line(p1,p2,1.2f,ink.alpha(.9f));
        line(p2,p3,1.1f,ink.alpha(.75f));
        line(at(spine*.82f,.2f),at(spine*.6f,4.8f),1.7f,h.trim.energy(1.2f));
        line(at(spine*.72f,.2f),at(spine*.52f,4.8f),1.1f,h.trim.energy(.8f));
        const Vec lamp=at(spine*.66f,5.4f);
        halo(lamp,6,h.trim.alpha(.28f));
        ellipse(lamp,{1.2f,1.2f},h.trim.energy(2.6f));
        for (int s=0;s<3;++s) line(at(spine*(.12f+s*.1f),-3.2f),at(spine*(.12f+s*.1f),5.2f),.9f,ink.alpha(.62f));
        // Belt, buckle and pouches.
        polygon(at(1.2f,-6.f),at(1.2f,6.4f),at(4.4f,6.4f),at(4.4f,-6.f),h.leather);
        line(at(1.2f,-6.f),at(1.2f,6.4f),.9f,ink); line(at(4.4f,-6.f),at(4.4f,6.4f),.9f,ink);
        polygon(at(2.3f,4.5f),at(2.3f,6.7f),at(3.9f,6.7f),at(3.9f,4.5f),h.trim.energy(1.1f));
        for (int k=0;k<2;++k) {
            const float v=-.4f+k*3.9f;
            polygon(at(1.f,v),at(1.f,v+3.4f),at(-3.2f,v+3.4f),at(-3.2f,v),h.leather.energy(1.15f));
            line(at(.2f,v),at(.2f,v+3.4f),.7f,ink);
            line(at(-3.2f,v),at(-3.2f,v+3.4f),.8f,ink);
        }
        // Cross strap.
        line(at(spine*.97f,-1.4f),at(spine*.32f,5.8f),2.1f,ink);
        line(at(spine*.97f,-1.4f),at(spine*.32f,5.8f),1.3f,h.leather.energy(1.3f));
        ellipse(at(spine*.62f,2.4f),{1.4f,1.1f},h.edge);
    }
    ellipse(at(spine*1.02f,1.f),{5.f,3.1f},ink);
    ellipse(at(spine*1.02f,1.f),{4.2f,2.4f},h.suit.energy(.75f));

    // Head: neck, face, headgear and expression.
    const float look=clamp(h.aim.y,-.7f,.7f);
    const Vec head=neck+Vec{tf*(2.2f+h.crouch*1.8f)-h.flinch*1.6f*tf,-5.8f-h.pelvis*.4f-breathe*.25f+look*1.1f+squash*.4f};
    line(neck+Vec{0,-1.f},head+Vec{0,4.f},4.6f,ink);
    line(neck+Vec{0,-1.f},head+Vec{0,4.f},3.1f,h.skin.energy(.8f));
    const float hx=head.x,hy=head.y;
    auto onFace=[&](float forwardOffset,float y) { return Vec{hx+tf*forwardOffset*fa+(tf<0?-1:1)*0.f,hy+y}; };
    auto hairBack=[&]() {
        if (h.look==Look::rook) {
            for (int k=0;k<9;++k) {
                const float a=pi+k*pi/8.f;
                const Vec c=head+Vec{std::cos(a)*6.4f-tf*1.4f*fa,std::sin(a)*6.f-.8f};
                ellipse(c,{3.1f,3.1f},ink);
            }
            for (int k=0;k<9;++k) {
                const float a=pi+k*pi/8.f;
                const Vec c=head+Vec{std::cos(a)*6.4f-tf*1.4f*fa,std::sin(a)*6.f-.8f};
                ellipse(c,{2.5f,2.5f},mix(h.hair,hex(0x2a2a3a),(k%3)*.18f));
            }
        }
    };
    hairBack();
    ellipse(head,{6.7f,7.f},ink);
    ellipse(head,{5.9f,6.3f},h.skin);
    ellipse(head+Vec{tf*1.6f*fa,3.2f},{4.4f,3.2f},h.skin.energy(.9f));
    // Ear.
    const Vec ear=onFace(-2.4f,.9f)+Vec{(1-fa)*(-5.2f),0};
    ellipse(ear,{1.5f,2.f},h.skin.energy(.85f));
    // Eyes, brows, nose and mouth.
    const float ft=tf<0?-1.f:1.f;
    const Vec eyeA=Vec{hx+ft*3.f*fa+(1-fa)*2.3f,hy-.3f-look*.9f};
    const Vec eyeB=Vec{hx+ft*3.f*fa-(1-fa)*2.3f,hy-.3f-look*.9f};
    auto eye=[&](Vec p) {
        ellipse(p,{1.7f,1.25f},hex(0xf1f1ee));
        ellipse(p+Vec{ft*.55f*fa,0},{.85f,.95f},hex(0x1a1511));
        line(p+Vec{-2.1f,-2.1f},p+Vec{2.1f,-1.8f},.9f,h.hair.energy(.9f));
    };
    const bool helmeted=h.look==Look::soldier;
    if (!helmeted) {
        eye(eyeA);
        if (fa<.72f) eye(eyeB);
        if (fa>.35f) polygon(Vec{hx+ft*5.4f*fa,hy-.4f},Vec{hx+ft*7.7f*fa,hy+1.8f},Vec{hx+ft*5.2f*fa,hy+2.3f},Vec{hx+ft*4.6f*fa,hy+.4f},h.skin.energy(.72f));
        else line(Vec{hx,hy+.4f},Vec{hx,hy+2.2f},1.f,h.skin.energy(.65f));
        line(Vec{hx+ft*2.2f*fa-(1-fa)*1.4f,hy+3.9f},Vec{hx+ft*4.6f*fa+(1-fa)*1.4f,hy+3.9f},.9f,ink.alpha(.7f));
    }

    switch (h.look) {
    case Look::voss: {
        // Cropped hair, orange goggles on the headband, an ear cup and a scar.
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
        ellipse(ear,{2.1f,2.6f},ink); ellipse(ear,{1.5f,2.f},h.shadow.energy(1.5f));
        line(ear+Vec{-.8f,-.8f},ear+Vec{.8f,-.8f},.7f,h.trim.energy(2.f));
        line(ear+Vec{-.8f,.6f},ear+Vec{.8f,.6f},.7f,h.trim.energy(2.f));
        if (fa>.4f) line(Vec{hx+ft*3.8f*fa,hy+1.2f},Vec{hx+ft*4.7f*fa,hy+3.4f},.9f,hex(0xb85a50).alpha(.85f));
        break;
    }
    case Look::rook: {
        // Curly hair, a cyan monocle and a hood collar.
        for (int k=0;k<5;++k) {
            const float a=-pi*.92f+k*pi*.21f;
            const Vec c=head+Vec{std::cos(a)*5.6f*(1-.25f*fa)+ft*1.2f*fa,std::sin(a)*5.4f-1.2f};
            ellipse(c,{2.5f,2.4f},ink);
            ellipse(c,{2.f,1.9f},h.hair);
        }
        ellipse(eyeA,{2.7f,2.4f},ink);
        ellipse(eyeA,{2.1f,1.8f},h.trim.energy(1.7f).alpha(.82f));
        line(eyeA+Vec{-2.f*ft,-.4f},ear+Vec{0,-1.2f},.8f,h.edge.alpha(.8f));
        halo(eyeA,8,h.trim.alpha(.25f));
        ellipse(ear,{2.f,2.5f},ink); ellipse(ear,{1.4f,1.9f},h.shadow.energy(1.4f));
        break;
    }
    case Look::vale: {
        // Swept grey hair, beard, goggles on the forehead and a headset with a microphone.
        ellipse(head+Vec{-ft*1.6f*fa,-3.3f},{6.6f,4.2f},ink);
        ellipse(head+Vec{-ft*1.6f*fa,-3.4f},{5.9f,3.6f},h.hair);
        for (int k=0;k<4;++k) line(head+Vec{-ft*(4.8f-k*1.5f)*fa,-6.3f+k*.4f},head+Vec{ft*(1.2f+k*.8f)*fa,-5.2f+k*.8f},.8f,h.hair.energy(1.4f));
        ellipse(head+Vec{ft*2.f*fa,4.2f},{5.1f,3.7f},ink);
        ellipse(head+Vec{ft*2.f*fa,4.1f},{4.5f,3.1f},hex(0x9aa0aa));
        line(Vec{hx+ft*2.2f*fa,hy+3.1f},Vec{hx+ft*5.4f*fa,hy+3.1f},1.1f,hex(0x70757f));
        line(head+Vec{-6.4f,-4.4f},head+Vec{6.4f,-4.4f},2.2f,ink);
        line(head+Vec{-6.2f,-4.4f},head+Vec{6.2f,-4.4f},1.3f,h.leather.energy(1.2f));
        const Vec lens=head+Vec{ft*2.4f*fa,-4.9f};
        ellipse(lens,{3.4f+(1-fa)*1.2f,2.3f},ink);
        ellipse(lens,{2.7f+(1-fa)*1.2f,1.7f},hex(0x8a78e0).energy(1.5f));
        line(lens+Vec{-1.4f,-.6f},lens+Vec{.6f,-.9f},.7f,hex(0xffffff).alpha(.7f));
        ellipse(ear,{2.4f,2.9f},ink); ellipse(ear,{1.8f,2.3f},h.shadow.energy(1.5f));
        ellipse(ear+Vec{0,-.6f},{.8f,.8f},h.trim.energy(2.5f));
        const Vec m0=ear+Vec{ft*1.f,1.6f}, m1=Vec{hx+ft*2.5f*fa,hy+5.3f}, m2=Vec{hx+ft*5.2f*fa,hy+4.5f};
        line(m0,m1,.9f,ink); line(m1,m2,.9f,ink);
        ellipse(m2,{1.1f,1.1f},h.trim.energy(1.6f));
        break;
    }
    case Look::soldier: {
        // Closed helmet with a glowing visor and a chin guard.
        ellipse(head+Vec{0,-1.2f},{7.4f,6.7f},ink);
        ellipse(head+Vec{0,-1.3f},{6.6f,5.9f},h.shell.energy(1.05f));
        polygon(head+Vec{-6.2f,-.6f},head+Vec{6.2f,-.6f},head+Vec{5.6f,2.6f},head+Vec{-5.6f,2.6f},h.suit.energy(.9f));
        const Vec visorA=Vec{hx+ft*(-1.2f)*fa-(1-fa)*4.6f,hy-.4f}, visorB=Vec{hx+ft*6.4f*fa+(1-fa)*4.6f,hy-.4f};
        line(visorA,visorB,3.6f,ink);
        line(visorA,visorB,2.3f,h.visor.energy(2.4f));
        halo((visorA+visorB)*.5f,10,h.visor.alpha(.32f));
        line(head+Vec{-4.8f,-5.f},head+Vec{3.4f,-6.2f},1.1f,h.edge.alpha(.85f));
        line(head+Vec{ft*2.f*fa,-6.4f},head+Vec{ft*2.f*fa,-.4f},.9f,ink.alpha(.6f));
        polygon(head+Vec{-2.f*ft,3.4f},head+Vec{ft*5.2f*fa,3.4f},head+Vec{ft*4.6f*fa,6.4f},head+Vec{-ft*1.4f,6.6f},h.shell.energy(.8f));
        line(head+Vec{-ft*2.2f,3.6f},head+Vec{ft*4.8f*fa,3.6f},.8f,ink);
        ellipse(ear,{2.1f,2.6f},ink); ellipse(ear,{1.5f,2.f},h.shadow.energy(1.5f));
        break;
    }
    }

    // Shoulders. The rest of the figure is drawn out of any flip rotation so the weapon always follows the aim.
    Vec shoulder=neck+Vec{-1.f*tf-counter*(2.2f+h.run)*dir,2.6f+squash*.2f};
    Vec farShoulder=shoulder+Vec{-2.4f*tf,.4f};
    if (h.spin!=0) { shoulder=transform(shoulder); farShoulder=transform(farShoulder); endSpin(); }

    Vec nearHand=h.nearHand, farHand=h.farHand;
    if (h.gripFromChest) {
        const Vec mount=shoulder+Vec{0,5.f};
        nearHand=mount+h.aim*(h.gripNear-h.recoil*3.f);
        farHand=mount+h.aim*(h.gripFar-h.recoil*3.f);
    }
    if (h.farFree) {
        const float swing=-counter;
        farHand=farShoulder+Vec{dir*swing*(6.f+h.run*9.f),15.f-h.run*3.f-std::abs(swing)*h.run*2.f};
    }

    auto arm=[&](Vec s,Vec hand,bool near,bool elbowDown) {
        constexpr float upper=11.6f,lower=10.6f;
        const Vec a=solveKnee(s,hand,upper,lower,1), b=solveKnee(s,hand,upper,lower,-1);
        const Vec elbow=elbowDown?(a.y>b.y?a:b):(a.x*dir<b.x*dir?a:b);
        const Color sleeve=near?h.suit:h.shadow;
        const Color plate=near?h.shell:h.shadow.energy(1.3f);
        const Vec toHand=normal(hand-elbow);
        armor(s,elbow,near?4.1f:3.2f,near?2.8f:2.3f,sleeve,h.edge,near);
        armor(elbow,hand-toHand*1.4f,near?2.9f:2.4f,near?2.1f:1.9f,sleeve.energy(.95f),h.edge,near);
        armor(mix(elbow,hand,.36f),mix(elbow,hand,.9f),near?3.3f:2.7f,near?2.8f:2.3f,plate,h.edge,near);
        ellipse(elbow,{near?2.7f:1.8f,near?2.6f:1.7f},ink);
        ellipse(elbow,{near?2.0f:1.2f,near?1.9f:1.1f},plate.energy(near?1.15f:.95f));
        if (near) {
            const Vec light=mix(elbow,hand,.64f);
            ellipse(light,{1.3f,.9f},h.trim.energy(2.4f));
            line(mix(elbow,hand,.42f)-Vec{0,1.8f},mix(elbow,hand,.42f)+Vec{0,1.8f},.8f,ink.alpha(.6f));
        }
        // Pauldron over the joint.
        const Color cap=near?(h.look==Look::voss?h.trim.energy(.95f):h.shell.energy(1.12f)):h.shadow.energy(1.5f);
        const Vec armAxis=normal(elbow-s), armSide{armAxis.y,-armAxis.x};
        ellipse(s,{near?4.8f:3.7f,near?4.3f:3.3f},ink);
        ellipse(s,{near?4.0f:2.9f,near?3.5f:2.5f},cap);
        if (near) {
            line(s-armSide*3.2f+armAxis*.5f,s+armSide*3.2f+armAxis*.5f,.9f,h.edge.alpha(.85f));
            line(s-armSide*3.6f+armAxis*2.6f,s+armSide*3.6f+armAxis*2.6f,1.3f,h.look==Look::voss?ink.alpha(.75f):h.trim.energy(1.1f));
        }
        ellipse(hand,{near?3.2f:2.7f,near?3.f:2.5f},ink);
        ellipse(hand,{near?2.5f:2.1f,near?2.3f:1.9f},h.glove);
        line(hand+Vec{-.8f,.9f},hand+Vec{.9f,1.2f},.7f,ink.alpha(.8f));
    };
    const bool gunArms=!h.farFree;
    arm(farShoulder,farHand,false,gunArms);
    if (h.weapon) h.weapon(nearHand,farHand);
    arm(shoulder,nearHand,true,true);
}

void Renderer::drawPlayer(const Game& g) {
    using Look=Humanoid::Look;
    const auto& p=g.player;
    const Vec shift=offset+playerShift;
    const Vec ground{p.box.center().x+shift.x,p.box.bottom()+shift.y};
    const int index=int(g.character-characters.data());
    const Color trim=hex(g.character->color);
    const bool hurtBlink=p.flash>0 && std::sin(p.flash*80)>0;

    struct Palette { uint32_t suit,shell,skin,hair,leather,glove; };
    static constexpr Palette palettes[]{
        {0x323b48,0x566474,0xd5a07c,0x2e211b,0x4a382c,0x232832},
        {0x234650,0x3a6c7a,0x9a6a4d,0x15141c,0x4d3d30,0x1c2d33},
        {0x2f2d49,0x4d4a72,0xc79b78,0x8d919c,0x433a33,0x23212f}
    };
    const auto& pal=palettes[std::clamp(index,0,2)];
    Humanoid h;
    h.look=static_cast<Look>(std::clamp(index,0,2));
    h.ground=ground; h.facing=p.facing; h.turn=p.turn;
    const bool moving=std::abs(p.velocity.x)>25;
    h.stride=moving?pose.direction:p.facing;
    for (int i=0;i<2;++i) { h.heels[i]=drawHeels[i]+offset; h.footAngle[i]=drawFootAngles[i]; }
    h.grounded=p.support>=0;
    h.lean=pose.lean; h.crouch=pose.crouch; h.run=pose.run; h.weight=pose.weight; h.cycle=pose.cycle;
    h.compression=pose.compression; h.pelvis=pose.pelvis; h.tuck=pose.tuck;
    h.time=g.time; h.velocity=p.velocity; h.landed=p.landed;
    h.flinch=smooth((p.flash-.45f)/.24f);
    h.spin=p.flip>0?p.flipDirection*2*pi*smooth(1-p.flip/flipSeconds):0.f;
    h.scarf=true;
    h.suit=hurtBlink?hex(0xd9a79a):hex(pal.suit);
    h.shell=hurtBlink?hex(0xffffff):hex(pal.shell);
    h.trim=trim; h.edge=mix(trim,hex(0xffffff),.7f); h.shadow=hex(0x1d2430).energy(1.1f);
    h.ink=hex(0x06080d); h.skin=hex(pal.skin); h.hair=hex(pal.hair);
    h.leather=hex(pal.leather); h.glove=hex(pal.glove); h.visor=trim;

    drawTrails(g,ground);

    // Weapon geometry in screen space.
    const float forwardFacing=p.facing>0?1.f:-1.f;
    const float counter=std::sin(pose.cycle*2*pi)*pose.weight;
    const float local=std::atan2(std::sin(p.aim),std::abs(std::cos(p.aim)));
    const float barrel=mix(local,.53f+counter*.13f,carry);
    const Vec aim{std::cos(barrel)*forwardFacing,std::sin(barrel)};
    const Vec side{-aim.y,aim.x};
    const Vec up=forwardFacing>0?Vec{aim.y,-aim.x}:Vec{-aim.y,aim.x};
    const float recoil=p.muzzle>0?3.7f*p.muzzle/.069f:0;
    const float sway=std::sin(g.time*1.7f)*.35f*(1-pose.weight);
    const Vec mount=g.weaponPivot()+shift+Vec{-carry*5.f*p.facing,(pose.crouch-p.gait.crouch)*12+carry*7.f+pose.pelvis*.5f+sway};
    const Vec muzzle=(carry>0?mount+aim*muzzleReach[g.gun]:g.muzzle()+shift+Vec{0,pose.pelvis*.5f+sway})-aim*(recoil*.45f);
    const bool art=g.gun<3;
    const GunArt& model=gunArt[art?g.gun:0];
    const float gunLength=model.length,gunHeight=model.length*model.aspect;
    const Vec axisU=aim*gunLength,axisV=up*(-gunHeight);
    h.aim=aim; h.recoil=recoil/3.7f;
    h.nearHand=art?muzzle+axisU*(model.gripU-1)+axisV*(model.gripV-model.muzzleV):mount+aim*14-aim*recoil;
    h.farHand=art?muzzle+axisU*(model.foreU-1)+axisV*(model.foreV-model.muzzleV):mount+aim*22+side*(2.3f*p.facing);
    const Color gunColor=hex(weapons[g.gun].color);
    h.weapon=[&](Vec hand,Vec) {
        if (art) { sprite(model.name,muzzle,axisU,axisV,{1.f,model.muzzleV}); return; }
        const Vec receiver=hand-aim*7.3f, fore=muzzle-aim*3.5f;
        armor(receiver,fore,2.25f,1.65f,hex(0x475965),hex(0xb1bec0),true);
        line(fore,muzzle,2.55f,hex(0x96a8b3));
        line(receiver+aim*2-side*(1.75f*p.facing),fore-side*(1.75f*p.facing),.95f,gunColor.energy(1.41f));
        line(hand,hand+side*(5.2f*p.facing),2.6f,hex(0x263640));
    };
    humanoid(h);

    if (p.muzzle>0) {
        const float flash=p.muzzle/.069f;
        const Vec spread=side*(5.6f+flash*7.f),tip=muzzle+aim*(11+flash*22);
        halo(muzzle,52,gunColor.energy(2.2f).alpha(flash*.70f));
        triangle(muzzle-spread,tip,muzzle+spread,gunColor.energy(2.8f).alpha(.92f));
        triangle(muzzle-spread*.3f,tip-aim*2,muzzle+spread*.3f,hex(0xfff6df).energy(3.1f));
        for (int i=-1;i<=1;i+=2) {
            const Vec dir=normal(aim+side*(.62f*i));
            triangle(muzzle-side*(1.6f*i),muzzle+dir*(10+flash*12),muzzle+side*(1.6f*i)*.2f,gunColor.energy(2.6f).alpha(.85f));
        }
        ellipse(muzzle+aim*2,{4.2f+flash*3.f,4.2f+flash*3.f},hex(0xfff6df).energy(3.2f));
    }
}

void Renderer::drawCorpses(const Game& g) {
    const Color accent=hex(g.stage->accent);
    for (const auto& c:g.corpses) {
        if (!visible(c.body.box.x,c.body.box.w)) continue;
        const float remaining=c.life-c.age;
        if (remaining<.4f && std::fmod(c.age*24,2.f)<1.f) continue;
        const Vec pivot=c.body.box.center()+offset;
        beginSpin(pivot,c.angle);
        drawEnemy(c.body,accent,g.index,g.time,pivot);
        endSpin();
    }
}

void Renderer::drawEnemy(const Enemy& e,Color accent,int sector,float time,Vec target) {
    const Rect b=e.box;
    const Vec shove{e.kick*e.flinch*4.f,0};
    const Vec center=b.center()+offset+shove;
    const float x=center.x,y=center.y;
    const int facing=e.facing;
    const float turn=e.turn;
    const Color alert=hex(0xff6870);
    const Color rim=e.hit>0?hex(0xffffff):mix(hex(0x9fb0b8),accent,.35f);
    const Color hull=e.hit>0?hex(0xfff0d8):mix(hex(0x39424e),accent,.24f);
    const Color black=hex(0x06080d);
    const Color plate=e.hit>0?hex(0xffffff):mix(hex(0x56626f),accent,.3f);
    const float pace=clamp(std::abs(e.vx)/96);
    const float breath=std::sin(time*2.1f+e.phase);

    if (e.kind==EnemyKind::drone) {
        // Hovering scout. It banks into its movement, spins four-blade rotors and watches the player.
        const float bank=std::cos(time*1.42f+e.phase)*.2f*(e.alive?1.f:0.f);
        const Vec at{x,y+std::sin(time*7.f+e.phase)*.8f};
        const float ca=std::cos(bank),sa=std::sin(bank);
        auto local=[&](float lx,float ly) { return at+Vec{lx*ca-ly*sa,lx*sa+ly*ca}; };
        for (float side:{-1.f,1.f}) {
            const Vec mount=local(side*19.f,-2.f), rotor=local(side*22.f,-7.f);
            line(local(side*8.f,0),mount,3.4f,black);
            line(local(side*8.f,0),mount,2.1f,hull);
            ellipse(rotor,{4.f,3.f},black);
            const float flutter=std::sin(time*70.f+side)*2.f;
            ellipse(rotor,{13.f+flutter,1.9f},rim.alpha(.40f));
            line(rotor-Vec{14.f+flutter,0},rotor+Vec{14.f+flutter,0},1.1f,rim.alpha(.85f));
            halo(local(side*22.f,5.f),20,accent.alpha(.38f));
            line(local(side*22.f,-1.f),local(side*22.f,7.f),2.4f,accent.energy(1.9f));
        }
        for (float side:{-1.f,1.f}) {
            line(local(side*7.f,8.f),local(side*11.f,14.f),1.6f,black);
            line(local(side*11.f,14.f),local(side*13.5f,14.f),1.6f,black);
        }
        ellipse(at,{18.f,12.5f},black);
        ellipse(at+Vec{0,-.6f},{16.f,10.7f},hull);
        polygon(local(-14.f,-3.f),local(14.f,-3.f),local(12.f,-7.5f),local(-12.f,-7.5f),plate);
        line(local(-12.f,3.f),local(12.f,3.f),.9f,black.alpha(.8f));
        for (int k=-2;k<=2;++k) line(local(k*4.f,4.f),local(k*4.f+1.f,9.f),.9f,black.alpha(.6f));
        line(local(-13.f,-4.5f),local(13.f,-4.5f),1.f,rim.alpha(.8f));
        line(local(0,-10.f),local(2.f,-17.f),1.1f,black);
        ellipse(local(2.f,-17.5f),{1.3f,1.3f},alert.energy(2.f+std::sin(time*5.f)));
        const Vec look=normal(target-at);
        const Vec lens=at+Vec{facing*6.f*std::abs(turn),0}+look*1.6f;
        ellipse(lens,{5.4f,5.1f},black);
        ellipse(lens,{4.2f,4.f},hex(0x2a1215));
        emissive(lens+look*.8f,2.5f,alert,e.warning>0?3.2f:2.f);
        ellipse(lens+look*1.4f,{1.f,1.f},hex(0xfff0e0).energy(2.4f));
        halo(local(0,9.f),12,accent.alpha(.22f));
    } else if (e.kind==EnemyKind::turret) {
        // A servo turret on a splayed tripod. The gimbal tracks smoothly and the barrel kicks back.
        const Vec base{x,b.bottom()+offset.y};
        for (float side:{-1.f,0.f,1.f}) {
            const Vec foot=base+Vec{side*26.f,0};
            const Vec knee=base+Vec{side*15.f,-15.f+std::abs(side)*3.f};
            line(foot,knee,5.f,black); line(knee,center+Vec{side*6.f,10.f},5.f,black);
            line(foot,knee,3.2f,hull); line(knee,center+Vec{side*6.f,10.f},3.2f,hull);
            ellipse(knee,{2.5f,2.5f},plate);
            line(foot-Vec{3.f,0},foot+Vec{3.f,0},2.4f,black);
        }
        polygon(base+Vec{-20.f,0},base+Vec{20.f,0},base+Vec{14.f,-7.f},base+Vec{-14.f,-7.f},hull.energy(.8f));
        line(base+Vec{-20.f,0},base+Vec{20.f,0},1.f,rim.alpha(.6f));
        const Vec base2=center+Vec{0,8.f};
        ellipse(base2,{14.f,6.f},black); ellipse(base2,{12.f,4.6f},plate);
        const float idleAngle=facing>0?-.15f:pi+.15f;
        Vec dirv=e.warning>0?ray(e.aim):normal(mix(ray(idleAngle),normal(target-center),smooth(e.raise)));
        if (!e.alive) dirv=ray(idleAngle+(facing>0?.9f:-.9f));
        const Vec perp{-dirv.y,dirv.x};
        const float kick=e.recoil*6.f;
        ellipse(center,{15.5f,13.f},black);
        ellipse(center-Vec{0,1.f},{13.f,10.6f},hull);
        ellipse(center-Vec{0,3.f},{10.f,5.f},plate);
        for (int k=-1;k<=1;k+=2) {
            const Vec o=perp*(4.2f*k);
            line(center+dirv*(4.f-kick)+o,center+dirv*(25.f-kick)+o,3.6f,black);
            line(center+dirv*(4.f-kick)+o,center+dirv*(25.f-kick)+o,2.2f,rim.energy(.75f));
            line(center+dirv*(23.f-kick)+o,center+dirv*(28.f-kick)+o,4.2f,black);
            line(center+dirv*(24.f-kick)+o,center+dirv*(26.5f-kick)+o,2.4f,plate);
        }
        line(center+dirv*8.f-perp*7.f,center+dirv*8.f+perp*7.f,2.4f,black.alpha(.8f));
        // Ammunition belt feeding the barrels.
        for (int k=0;k<5;++k) {
            const Vec bead=center-perp*8.f-dirv*(2.f+k*3.f)+Vec{0,k*1.5f};
            ellipse(bead,{1.3f,1.3f},hex(0xf0c46b).energy(1.1f));
        }
        const Vec eye=center+perp*(-7.f)+dirv*5.f;
        ellipse(eye,{2.4f,2.4f},black);
        emissive(eye,1.9f,alert,e.warning>0?3.1f:1.4f);
        emissive(center,3.2f,alert,e.warning>0?3.1f:1.4f);
        if (e.recoil>.5f) halo(center+dirv*30.f,18,accent.energy(2.f).alpha(.4f*e.recoil));
    } else if (e.kind==EnemyKind::stalker) {
        // Quadruped hunter. Diagonal gait, an undulating spine, a lashing tail and a crouch before the lunge.
        const float squash=e.landed*3.f;
        const float lunge=clamp(-e.vertical/450,0,1), fall=clamp(e.vertical/450,0,1);
        const float phase=e.distance*.07f;
        const float run=pace;
        const Vec ground{x,b.bottom()+offset.y};
        const Vec body=center+Vec{0,squash+(e.vertical==0?std::cos(phase*2)*1.2f*run:0.f)};
        const float dirx=turn;
        const float pitch=(lunge*-.2f+fall*.25f)*dirx;
        auto bodyAt=[&](float lx,float ly) { return body+Vec{lx*dirx,ly+lx*pitch*dirx}; };
        struct Leg { float lx; int pair; };
        const Leg legs[4]{{-14.f,1},{-9.f,0},{9.f,0},{14.f,1}};
        for (int pass=0;pass<2;++pass) {
            for (int i=0;i<4;++i) {
                const bool nearSide=(i%2)==1;
                if ((pass==0)==nearSide) continue;
                const auto& leg=legs[i];
                const float cyc=phase+leg.pair*pi;
                const float swing=run>.05f?std::sin(cyc):0.f;
                const float rise=run>.05f?std::max(0.f,std::cos(cyc)):0.f;
                Vec foot=ground+Vec{leg.lx*dirx*1.5f*(1+.2f*(i<2?1:-1))+swing*10.f*run*dirx,-rise*7.f*run};
                if (e.vertical!=0) foot=bodyAt(leg.lx*1.3f+(i<2?-1.f:1.f)*lunge*10.f,12.f+lunge*8.f-fall*4.f);
                const Vec hipJoint=bodyAt(leg.lx,3.f);
                const int knee=(i<2?-1:1)*(dirx>=0?1:-1);
                const Vec kneeP=solveKnee(hipJoint,foot,13.f,14.f,knee);
                const Color tint=nearSide?hull:hull.energy(.66f);
                limb(hipJoint,kneeP,nearSide?5.f:4.2f,tint,rim);
                limb(kneeP,foot,nearSide?4.2f:3.4f,tint.energy(.9f),rim.energy(.7f));
                line(foot-Vec{1.5f*dirx,0},foot+Vec{4.f*dirx,0},2.4f,black);
                line(foot+Vec{3.f*dirx,0},foot+Vec{5.5f*dirx,-1.2f},1.4f,rim);
                if (nearSide) ellipse(kneeP,{2.3f,2.3f},plate);
            }
        }
        // Tail with three trailing segments.
        Vec from=bodyAt(-17.f,-1.f);
        for (int k=1;k<=4;++k) {
            const Vec to=bodyAt(-17.f-k*6.5f,-2.f-k*1.4f+std::sin(time*6.f-k*.9f+e.phase)*2.2f*(1+run));
            limb(from,to,std::max(1.8f,5.2f-k*.8f),hull.energy(.8f),rim.energy(.6f));
            from=to;
        }
        triangle(from+Vec{-1.f*dirx,-2.f},from+Vec{-6.f*dirx,-4.f},from+Vec{-1.f*dirx,1.5f},accent.energy(1.8f));
        // Segmented chassis, dorsal vents and head pod.
        ellipse(body+Vec{0,0},{19.f,10.2f},black);
        ellipse(body+Vec{0,-1.f},{17.2f,8.f},hull);
        for (int k=-3;k<=3;++k) {
            const Vec seg=bodyAt(k*4.6f,-7.f);
            line(seg,seg+Vec{0,6.f},.9f,black.alpha(.55f));
            if (k%2==0) line(bodyAt(k*4.6f-1.4f,-8.f),bodyAt(k*4.6f+1.4f,-8.f),1.4f,accent.energy(1.3f+.4f*std::sin(time*5.f+k)));
        }
        line(bodyAt(-15.f,-5.5f),bodyAt(14.f,-6.f),1.f,rim.alpha(.7f));
        const Vec head=bodyAt(20.f,-1.f+breath*.6f+lunge*-2.f);
        ellipse(head,{9.5f,7.2f},black);
        ellipse(head+Vec{-.5f*dirx,-.6f},{8.2f,6.f},hull.energy(1.05f));
        for (int k=-1;k<=1;k+=2) {
            line(head+Vec{5.f*dirx,2.5f*k},head+Vec{12.f*dirx,5.f*k+std::sin(time*9.f)*.8f},1.8f,black);
            line(head+Vec{5.f*dirx,2.5f*k},head+Vec{11.f*dirx,4.6f*k+std::sin(time*9.f)*.8f},.9f,rim);
        }
        emissive(head+Vec{3.5f*dirx,-2.4f},2.1f,alert,e.hp<e.maxHp?2.6f:1.9f);
        emissive(head+Vec{6.f*dirx,-.8f},1.4f,alert,1.5f);
    } else if (e.kind==EnemyKind::ranger) {
        // Soldier built on the same rig as the player. Weapon low when idle, raised to aim at the target.
        Humanoid h;
        h.look=Humanoid::Look::soldier;
        const Vec ground{x,b.bottom()+offset.y+shove.y};
        h.ground=ground; h.facing=facing; h.stride=facing; h.turn=turn;
        const float cycle=std::fmod(e.distance/56,1.f);
        for (int i=0;i<2;++i) {
            const float c=std::fmod(cycle+.5f*i,1.f);
            const bool stance=c<.61f;
            const float u=stance?c/.61f:(c-.61f)/.39f;
            const float xFoot=(stance?mix(15.f,-14.f,u):mix(-14.f,15.f,smooth(u)))*pace*facing;
            const float rise=stance?0.f:std::sin(pi*u)*7.f*pace;
            h.heels[i]={x+xFoot+(i==0?-3.5f:3.5f),ground.y-rise};
            const float ang=stance?.26f*(1-smooth(u/.2f))-.34f*smooth((u-.7f)/.3f):mix(-.34f,.26f,smooth(u));
            h.footAngle[i]=ang*pace;
        }
        h.grounded=true;
        h.lean=clamp(e.vx/96*.15f,-.2f,.2f)-e.flinch*e.kick*.05f;
        h.run=pace*.55f; h.weight=pace; h.cycle=cycle;
        h.pelvis=std::cos(cycle*4*pi-.4f)*pace*1.3f;
        h.time=time+e.phase; h.velocity={e.vx,0}; h.landed=0; h.flinch=e.flinch;
        h.scarf=false; h.gripFromChest=true; h.farFree=false;
        const float lowAngle=facing>0?.7f:pi-.7f;
        const Vec chest{x,y-12.f};
        const float aimAngle=e.warning>0?e.aim:std::atan2(target.y-chest.y,target.x-chest.x);
        h.aim=e.warning>0?ray(e.aim):normal(mix(ray(lowAngle),ray(aimAngle),smooth(e.raise)));
        h.recoil=e.recoil;
        h.suit=e.hit>0?hex(0xffe9d0):mix(hex(0x2a323d),accent,.16f);
        h.shell=e.hit>0?hex(0xffffff):mix(hex(0x56626f),accent,.28f);
        h.trim=accent; h.edge=mix(hex(0xc4d0d6),accent,.3f); h.shadow=hex(0x1a2029);
        h.ink=black; h.skin=mix(hex(0x39424e),accent,.12f); h.hair=hex(0x20252d);
        h.leather=hex(0x3a342f); h.glove=hex(0x1f242c); h.visor=alert;
        h.weapon=[&](Vec hand,Vec foregrip) {
            const Vec aim=h.aim;
            Vec down{-aim.y,aim.x};
            if (down.y<0) down=down*-1.f;
            const Vec stock=hand-aim*13.f, tail=hand-aim*2.f, muzzle=hand+aim*25.f;
            armor(stock,tail,2.6f,2.8f,plate.energy(.8f),rim,true);
            armor(tail,hand+aim*10.f,3.1f,2.8f,hull.energy(.95f),rim,true);
            armor(hand+aim*8.f,muzzle-aim*5.f,2.3f,2.f,hull.energy(.7f),rim,true);
            line(muzzle-aim*5.f,muzzle,2.6f,black);
            line(muzzle-aim*5.f,muzzle,1.6f,rim.energy(.8f));
            polygon(hand+aim*2.f,hand+aim*5.5f,hand+aim*4.f+down*9.f,hand+aim*.5f+down*9.f,black);
            line(hand+aim*1.5f+down*1.5f,hand+aim*3.6f+down*8.2f,1.2f,plate);
            line(hand+aim*4.f-down*3.f,hand+aim*11.f-down*3.f,2.2f,black);
            line(hand+aim*5.f-down*3.f,hand+aim*10.f-down*3.f,1.1f,accent.energy(1.8f));
            line(hand+aim*14.f-down*3.f,hand+aim*14.f-down*5.5f,1.2f,black);
            line(hand-aim*2.f+down*.5f,hand+aim*12.f+down*.5f,.8f,accent.energy(e.warning>0?2.4f:1.2f));
            (void)foregrip;
        };
        humanoid(h);
    } else {
        // Sector guardians. Heavy articulated frames with piston legs, riveted plating and a breathing core.
        const Vec base{x,y+b.h*.5f};
        const Color core=sector==4?hex(0xa6c8ff):accent;
        const float stomp=std::sin(e.distance*.084f)*pace;
        const float breathe=1+breath*.05f;
        if (sector!=4) for (float side:{-1.f,1.f}) {
            const float gait=side*stomp*9.f;
            const float lift=std::max(0.f,-side*stomp)*7.f;
            const Vec hipJoint{x+side*24.f,y+11.f};
            const Vec foot{base.x+side*38.f+gait,base.y-2.f-lift};
            const Vec kneeP=solveKnee(hipJoint,foot,24.f,25.f,-int(side));
            limb(hipJoint,kneeP,15,hull,rim); limb(kneeP,foot,12,hull.energy(.76f),rim);
            line(kneeP+Vec{-side*7.f,0},kneeP+Vec{-side*7.f,10.f},4.f,black);
            line(kneeP+Vec{-side*7.f,1.f},kneeP+Vec{-side*7.f,9.f},2.f,plate);
            ellipse(kneeP,{8.f,8.f},black); ellipse(kneeP,{6.4f,6.4f},plate);
            ellipse(kneeP,{2.4f,2.4f},core.energy(1.8f));
            polygon(foot+Vec{-12.f,-4.f},foot+Vec{13.f,-4.f},foot+Vec{15.f,3.f},foot+Vec{-14.f,3.f},black);
            polygon(foot+Vec{-10.f,-3.f},foot+Vec{11.f,-3.f},foot+Vec{12.f,1.5f},foot+Vec{-11.f,1.5f},hull.energy(.9f));
            for (int t=-1;t<=1;++t) line(foot+Vec{t*5.f,-3.f},foot+Vec{t*5.f,2.f},.9f,black.alpha(.7f));
        }
        switch (sector) {
        case 0:
            gradient({x-37,y-32,74,63},hull.energy(1.3f),hull.energy(.54f));
            outline({x-37,y-32,74,63},black,2.5f);
            for (float side:{-1.f,1.f}) {
                limb(center+Vec{side*31,-20},center+Vec{side*48,17},12,hull,rim);
                line(center+Vec{side*48,17},center+Vec{side*51,28},5,black);
                ellipse(center+Vec{side*31,-20},{9.f,8.f},black); ellipse(center+Vec{side*31,-20},{7.5f,6.5f},plate);
            }
            for (int i=0;i<4;++i) rect({x-29+i*5.f,y-23,2,11},core.energy(1.23f));
            for (int i=0;i<2;++i) for (int j=0;j<5;++j) ellipse({x-31.f+j*15.f,y-28.f+i*54.f},{1.3f,1.3f},black);
            for (int i=0;i<6;++i) line({x-35.f+i*12.f,y+27.f},{x-27.f+i*12.f,y+16.f},3.f,accent.energy(.9f));
            line({x-37.f,y+2.f},{x+37.f,y+2.f},1.1f,black.alpha(.8f));
            break;
        case 1:
            ellipse(center,{49,34},black);
            ellipse(center-Vec{0,2},{45,29},hull);
            for (int i=-1;i<=1;++i) {
                const Vec cap{x+i*28.f,y-26};
                limb(cap,cap-Vec{0,10},9,hull,rim);
                line(cap-Vec{4,12},cap+Vec{4,-12},2.2f,core.energy(1.65f));
            }
            line({x-39,y+12},{x+39,y+12},2,rim);
            for (int i=-3;i<=3;++i) ellipse({x+i*12.f,y+19.f},{1.5f,1.5f},black);
            for (int i=-2;i<=2;++i) line({x+i*14.f,y-10.f},{x+i*14.f+4.f,y+8.f},1.f,black.alpha(.6f));
            break;
        case 2:
            ellipse(center,{39,37},black);
            for (int i=0;i<9;++i) {
                const Vec d=ray(i*2*pi/9+std::sin(time*.8f+i)*.04f), side{-d.y,d.x};
                triangle(center+d*20-side*12,center+d*54,center+d*20+side*12,hull.energy(.98f));
                line(center+d*28,center+d*47,2.2f,core.energy(1.31f));
            }
            ellipse(center,{32,30},hull.energy(.87f));
            ellipse(center,{24,23},black);
            break;
        case 3:
            triangle(center-Vec{46,27},center+Vec{46,-27},center+Vec{0,43},hull);
            gradient({x-36,y-34,72,35},hull.energy(1.38f),hull.energy(.62f));
            for (float side:{-1.f,1.f}) {
                limb(center+Vec{side*24,-15},center+Vec{side*41,14},16,hull,rim);
                line(center+Vec{side*37,-5},center+Vec{side*43,7},3.4f,core.energy(2.1f));
            }
            for (int i=0;i<5;++i) line({x-30.f+i*15.f,y-30.f},{x-30.f+i*15.f,y-6.f},1.f,black.alpha(.55f));
            break;
        case 4:
            ellipse(center,{55,34},black);
            ellipse(center-Vec{0,3},{51,30},hull);
            line({x-63,y-4},{x+63,y-4},4,rim);
            for (float i:{-1.f,1.f}) line({x+i*31,y+21},{x+i*33,y+42},4,core.energy(1.85f));
            for (int i=-4;i<=4;++i) ellipse({x+i*11.f,y+8.f},{1.4f,1.4f},core.energy(1.2f+.5f*std::sin(time*3.f+i)));
            break;
        }
        const Vec dir=ray(e.warning>0?e.aim:facing>0?0:pi);
        const float kick=e.recoil*8.f;
        limb(center+Vec{facing*29.f,-4},center+dir*(56-kick)+Vec{0,-4},sector==3?14.f:9.f,hull,rim);
        ellipse(center+dir*(58-kick)+Vec{0,-4},{5.f,5.f},black);
        ellipse(center,{18,18},black);
        ellipse(center,{13*breathe,13*breathe},core.energy(.58f));
        emissive(center,8.7f*breathe,core,e.warning>0?3.5f:2.15f);
        ellipse(center,{3.3f,3.3f},hex(0xfff6df).energy(2.2f));
    }
    if (e.warning>0 && e.alive) {
        const Vec start{x,y-3}, dir=ray(e.aim);
        line(start,start+dir*99,1.1f,alert.energy(1.4f).alpha(.34f));
        halo(start,26,alert.energy(1.5f).alpha(.54f));
    }
    if (e.alive && e.kind!=EnemyKind::boss && e.hp<e.maxHp) {
        const Rect track{x-14,y-b.h*.5f-9,28,2.5f};
        rect(track,black);
        rect({track.x,track.y,track.w*clamp(float(e.hp)/e.maxHp),track.h},alert);
    }
}
}
