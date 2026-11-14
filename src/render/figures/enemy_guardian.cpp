#include "render/figures/enemy_view.hpp"

namespace vl {
// Sector guardians. Heavy articulated frames with piston legs, riveted plating and a breathing core.
// While dying the core flickers out, the hull smolders and the frame shudders and sinks (see drawEnemy).
void Renderer::drawGuardian(const EnemyView& v) {
    const Enemy& e=v.e;
    const Color black=v.black,hull=v.hull,rim=v.rim,plate=v.plate,alert=v.alert,accent=v.accent;
    const int sector=v.sector;
    const float time=v.time,x=v.x,y=v.y;
    const Vec center=v.center;
    const float flicker=e.alive?1.f:(1-v.burnt*.8f)*(std::sin(time*29.f)>-.1f?1.f:.2f);
    const Color core=(sector==4?hex(0xa6c8ff):accent).energy(flicker);
    const Rect b=e.box;
    const Vec base{x,y+b.h*.5f};
    const float stomp=std::sin(e.distance*.084f)*v.pace;
    const float breathe=1+v.breath*.05f;
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
        for (int i=0;i<6;++i) line({x-35.f+i*12.f,y+27.f},{x-27.f+i*12.f,y+16.f},3.f,accent.energy(.9f*(1-v.burnt)));
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
            const Vec d=ray(i*2*pi/9+std::sin(time*.8f+i)*.04f*(e.alive?1.f:0.f)), side{-d.y,d.x};
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
    const Vec dir=ray(e.warning>0?e.aim:v.facing>0?0:pi);
    const float kick=e.recoil*8.f;
    // The cannon droops as the frame loses power.
    const Vec aimDir=e.alive?dir:normal(dir+Vec{0,1.f}*smooth(v.dead/1.2f)*1.4f);
    limb(center+Vec{v.facing*29.f,-4},center+aimDir*(56-kick)+Vec{0,-4},sector==3?14.f:9.f,hull,rim);
    ellipse(center+aimDir*(58-kick)+Vec{0,-4},{5.f,5.f},black);
    ellipse(center,{18,18},black);
    ellipse(center,{13*breathe,13*breathe},core.energy(.58f));
    emissive(center,8.7f*breathe,core,e.warning>0?3.5f:2.15f);
    ellipse(center,{3.3f,3.3f},hex(0xfff6df).energy(2.2f*flicker));
    // Smoldering cracks across a dying hull.
    if (!e.alive) {
        for (int i=0;i<5;++i) {
            const Vec p=center+Vec{(i-2)*15.f,-14.f+std::sin(i*2.3f)*11.f};
            line(p,p+Vec{5.f,9.f},1.2f,hex(0xff8a3a).energy(1.6f*(1-v.burnt*.5f)));
        }
    }
    (void)alert;
}
} // namespace vl
