#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"
#include "render/figures/humanoid.hpp"

using namespace vl::figures;

namespace vl {
// Drawn weapons. Coordinates run forward from the muzzle (u is zero at the muzzle and negative toward the stock) and up (v).
void Renderer::gunModel(int gun,Vec muzzle,Vec aim,Vec up,float k,Color glow) {
    const Color ink=hex(0x06080d),steel=hex(0x7d8c9a),dark=hex(0x424c58),light=hex(0xc3cfd8);
    auto P=[&](float u,float v) { return muzzle+aim*(u*k)+up*(v*k); };
    auto block=[&](float u0,float u1,float v0,float v1,Color c,float inflate=.9f) {
        polygon(P(u0-inflate,v0-inflate),P(u1+inflate,v0-inflate),P(u1+inflate,v1+inflate),P(u0-inflate,v1+inflate),ink);
        polygon(P(u0,v0),P(u1,v0),P(u1,v1),P(u0,v1),c);
        line(P(u0+.3f,v1-.3f),P(u1-.3f,v1-.3f),.9f*k,c.energy(1.6f).alpha(.85f));
    };
    auto slanted=[&](Vec a,Vec b,Vec c,Vec d,Color col) {
        const Vec pa=P(a.x,a.y),pb=P(b.x,b.y),pc=P(c.x,c.y),pd=P(d.x,d.y);
        line(pa,pb,1.8f*k,ink); line(pb,pc,1.8f*k,ink); line(pc,pd,1.8f*k,ink); line(pd,pa,1.8f*k,ink);
        polygon(pa,pb,pc,pd,col);
    };
    auto glowLine=[&](float u0,float u1,float v,float w=1.f) { line(P(u0,v),P(u1,v),w*k,glow.energy(2.4f)); };
    switch (gun) {
    case 3:
        slanted({-9,-1},{-13.5f,-1},{-16.5f,-10.5f},{-12,-10.5f},dark);
        block(-15,-3,-1.6f,.2f,dark,.7f);
        block(-16,0,.2f,4.4f,steel);
        block(-2.4f,0,1.2f,3.2f,light,.6f);
        line(P(-9,-1.6f),P(-8.4f,-4.6f),1.2f*k,ink); line(P(-8.4f,-4.6f),P(-12.4f,-4.6f),1.2f*k,ink);
        block(-15.5f,-14,4.4f,5.6f,dark,.6f); block(-1.6f,-.6f,4.4f,5.4f,dark,.5f);
        for (int i=0;i<4;++i) line(P(-6.f-i*1.4f,.3f),P(-6.f-i*1.4f,4.1f),.6f*k,ink.alpha(.7f));
        glowLine(-12,-4,2.4f);
        break;
    case 4:
        line(P(-26,1.4f),P(-39,-1.8f),3.6f*k,ink); line(P(-26,1.4f),P(-39,-1.8f),2.2f*k,dark);
        slanted({-24,-2.6f},{-21,-2.6f},{-22.5f,-10},{-26,-10},dark);
        block(-26,-6,-2.6f,3.6f,steel);
        block(-24,-8,3.6f,5.2f,dark,.7f);
        block(-6,.6f,-.2f,3.2f,light);
        for (int i=0;i<3;++i) line(P(-4.8f+i*1.8f,-.2f),P(-4.8f+i*1.8f,3.2f),.7f*k,ink.alpha(.7f));
        block(-12,-6,-3.7f,-2.6f,dark,.6f);
        slanted({-19,-2.6f},{-14,-2.6f},{-12.6f,-15},{-17.6f,-15},dark);
        block(-10,-7.6f,-8.6f,-3.6f,dark,.7f);
        block(-18,-13,5.2f,7.6f,dark,.7f);
        ellipse(P(-13.5f,6.4f),{1.1f*k,1.4f*k},glow.energy(2.8f)); halo(P(-13.5f,6.4f),6*k,glow.alpha(.3f));
        glowLine(-24,-8,.5f); glowLine(-17.4f,-14.6f,-9.f,1.4f);
        break;
    case 5:
        block(-47,-36,-1,4.6f,dark);
        slanted({-31,-.6f},{-26.5f,-.6f},{-28,-10},{-32.5f,-10},dark);
        block(-36,0,-.6f,6.6f,steel);
        block(-3.6f,1,-1.5f,7.4f,light,.8f);
        block(-26,-23,-.6f,6.6f,glow.energy(.85f),.6f);
        ellipse(P(-18,-5.4f),{6.4f*k,6.4f*k},ink); ellipse(P(-18,-5.4f),{5.4f*k,5.4f*k},dark);
        for (int i=0;i<5;++i) { const Vec r=P(-18,-5.4f)+ray(i*2*pi/5+.4f)*(3.2f*k); ellipse(r,{1.3f*k,1.3f*k},glow.energy(1.6f)); }
        ellipse(P(-18,-5.4f),{1.2f*k,1.2f*k},steel);
        block(-13,-8.6f,-10,-.6f,dark,.7f);
        line(P(-9,6.6f),P(-9,10.4f),1.4f*k,ink); line(P(-9,10.4f),P(-12,10.4f),1.4f*k,ink);
        glowLine(-34,-28,2.8f,1.4f);
        break;
    default:
        block(-75,-59,-3.2f,4.8f,dark);
        slanted({-47,-3.6f},{-41.5f,-3.6f},{-43.5f,-13.4f},{-49,-13.4f},dark);
        block(-60,-30,-3.6f,7.8f,steel);
        block(-30,0,3.4f,5.8f,light,.8f);
        block(-30,0,-.2f,2.2f,light,.8f);
        glowLine(-30,0,2.8f,1.8f); halo(P(-4,2.8f),10*k,glow.alpha(.32f));
        for (int i=0;i<5;++i) block(-27+i*6.4f,-25.2f+i*6.4f,-1.6f,7.6f,dark,.55f);
        block(-53,-38,7.8f,11.2f,dark,.8f);
        ellipse(P(-38.5f,9.6f),{1.2f*k,1.8f*k},glow.energy(2.8f)); halo(P(-38.5f,9.6f),8*k,glow.alpha(.3f));
        block(-25,-19.5f,-9,-3.6f,dark,.7f);
        glowLine(-58,-33,.6f,1.2f); glowLine(-58,-33,4.6f,1.2f);
        break;
    }
}

void Renderer::showcaseGun(int gun,Rect box) {
    if (gun<3) {
        const auto& texture=asset(gunArt[gun].name);
        const float fit=std::min(box.w/texture.w,box.h/texture.h);
        image(gunArt[gun].name,{box.x+(box.w-texture.w*fit)*.5f,box.y+(box.h-texture.h*fit)*.5f,texture.w*fit,texture.h*fit});
        return;
    }
    static constexpr float lengths[]{0,0,0,26,38,50,76};
    const float k=std::min(box.w*.82f/lengths[gun],box.h*.80f/24.f);
    const Vec muzzle{box.x+box.w*.5f+lengths[gun]*k*.5f,box.y+box.h*.5f+3.f*k};
    gunModel(gun,muzzle,{1,0},{0,-1},k,hex(weapons[gun].color));
}
} // namespace vl
