#include "render/figures/humanoid.hpp"

namespace vl {

// A short ribbon in the operative's color. Its drift follows the velocity.
void Renderer::drawScarf(const Humanoid& h,const Rig& g) {
    const Vec base=g.neck+Vec{-2.5f*g.tf,2.f};
    const Vec wind{-clamp(h.velocity.x/420,-1.3f,1.3f)*4.6f-g.tf*2.4f,-clamp(h.velocity.y/760,-1.f,1.f)*3.2f};
    auto knot=[&](int i) {
        return base+Vec{wind.x*i*1.1f,i*1.05f+wind.y*i*.8f+std::sin(h.time*11-i*.85f)*i*.38f};
    };
    for (int pass=0;pass<2;++pass) {
        Vec from=base;
        for (int i=1;i<=8;++i) {
            const Vec to=knot(i);
            const float w=std::max(1.4f,5.8f-i*.55f);
            if (pass==0) line(from,to,w+2.6f,h.ink);
            else line(from,to,w,h.trim.energy(i%2?1.f:.8f));
            from=to;
        }
    }
}

// Backpack with gear that tells the operatives apart.
void Renderer::drawPack(const Humanoid& h,const Rig& g) {
    using Look=Humanoid::Look;
    const float spine=g.spine,tf=g.tf;
    const Color ink=h.ink;
    const Vec top=g.at(spine*.92f,-8.4f), bottom=g.at(spine*.16f,-7.6f);
    armor(top,bottom,4.9f,4.4f,h.shadow.energy(1.15f),h.edge.energy(.68f),true);
    line(g.at(spine*.72f,-8.4f)+Vec{-3.f,0},g.at(spine*.72f,-8.4f)+Vec{3.f,0},1.1f,ink.alpha(.8f));
    const Vec antenna=g.at(spine*.98f,-9.5f);
    if (h.cinematic) {
        // Stitched pack shell and separate rear vents, visible in three-quarter shots.
        for (int i=0;i<5;++i) {
            const float u=spine*(.24f+i*.11f);
            const Vec a=g.at(u,-10.4f),b=g.at(u,-8.7f);
            line(a,b,.72f,h.edge.energy(.91f).alpha(.85f));
        }
        const Vec latch=g.at(spine*.61f,-11.8f);
        ellipse(latch,{1.1f,1.5f},h.ink);
        ellipse(latch,{.65f,1.f},h.trim.energy(1.5f));
    }
    switch (h.look) {
    case Look::voss:
        armor(g.at(spine*.5f,-11.4f),g.at(spine*.3f,-11.4f),1.9f,1.9f,h.trim.energy(.7f),h.edge,true);
        line(antenna,antenna+Vec{-1.5f*tf,-15.f},1.1f,ink);
        ellipse(antenna+Vec{-1.5f*tf,-15.5f},{1.3f,1.3f},h.trim.energy(2.4f));
        break;
    case Look::rook:
        line(g.at(spine*1.02f,-8.6f)-Vec{5.5f,0},g.at(spine*1.02f,-8.6f)+Vec{5.5f,0},4.2f,ink);
        line(g.at(spine*1.02f,-8.6f)-Vec{5.f,0},g.at(spine*1.02f,-8.6f)+Vec{5.f,0},2.8f,h.leather.energy(1.4f));
        line(antenna+Vec{3.f,0},antenna+Vec{4.f*tf+3.f,-17.f},1.1f,ink);
        ellipse(antenna+Vec{4.f*tf+3.f,-17.5f},{1.4f,1.4f},h.trim.energy(2.6f));
        break;
    case Look::vale:
        armor(g.at(spine*.94f,-11.6f),g.at(spine*.2f,-11.6f),2.2f,2.2f,h.shadow.energy(1.5f),h.edge,true);
        for (int t=0;t<3;++t) ellipse(g.at(spine*(.78f-t*.14f),-11.9f),{1.1f,1.1f},h.trim.energy(2.5f));
        line(g.at(spine*.9f,-8.f),g.at(spine*1.16f,-1.f),1.8f,ink);
        line(g.at(spine*.9f,-8.f),g.at(spine*1.16f,-1.f),.9f,h.trim.energy(.9f));
        break;
    case Look::mercer: {
        // Oversized blast shield / hydraulic support frame: much wider than the standard rucksack.
        const Vec a=g.at(spine*.99f,-12.7f),b=g.at(spine*.08f,-12.7f);
        armor(a,b,8.2f,8.f,h.leather.energy(.85f),h.edge,true);
        for (int i=0;i<3;++i) {
            const Vec c=g.at(spine*(.22f+i*.25f),-15.7f);
            line(c-Vec{3.2f,0},c+Vec{3.2f,0},1.4f,ink);
            ellipse(c,{1.f,.9f},h.trim.energy(1.85f));
        }
        polygon(a+Vec{-5.f,-1.f},a+Vec{6.f,-1.f},a+Vec{6.f,4.5f},a+Vec{-5.f,4.5f},h.shell.energy(.92f));
        break;
    }
    case Look::cole: {
        // Light recon antenna and folding sensor-wing give a narrow, high silhouette.
        line(antenna,antenna+Vec{-6.5f*tf,-17.2f},1.1f,h.trim.energy(1.5f));
        const Vec sensor=antenna+Vec{-6.5f*tf,-18.f};
        triangle(sensor-Vec{8.f*tf,0},sensor+Vec{6.5f*tf,-2.5f},sensor+Vec{4.f*tf,2.6f},h.shell.energy(1.18f));
        ellipse(sensor,{1.7f,1.7f},h.trim.energy(2.1f));
        line(g.at(spine*.76f,-11.5f),g.at(spine*.15f,-14.f),2.8f,h.leather);
        break;
    }
    case Look::nyx: {
        // Symmetric pressure-cell pair with violet charge readouts.
        for (int side=-1;side<=1;side+=2) {
            const Vec a=g.at(spine*.95f,-12.1f)+Vec{side*4.2f,0};
            const Vec b=g.at(spine*.18f,-12.1f)+Vec{side*4.2f,0};
            armor(a,b,3.3f,3.3f,hex(0xcdc8d8),h.edge,true);
            armor(mix(a,b,.21f),mix(a,b,.78f),1.4f,1.4f,h.trim.energy(1.7f),h.edge,true);
            ellipse(b,{2.2f,1.7f},h.ink);
        }
        break;
    }
    case Look::soldier:
        line(antenna,antenna+Vec{-2.f*tf,-12.f},1.1f,ink);
        ellipse(antenna+Vec{-2.f*tf,-12.5f},{1.1f,1.1f},h.visor.energy(2.2f));
        break;
    }
}

// Abdomen, chest, chest plate with lamp, belt with pouches and the cross strap.
void Renderer::drawTorso(const Humanoid& h,const Rig& g) {
    const float spine=g.spine,breathe=g.breathe;
    const float bodyWidth=h.model?h.model->bodyWidth:1.f;
    const Color ink=h.ink;
    armor(g.waist,g.hip,5.9f*bodyWidth,5.4f*bodyWidth,h.suit.energy(.92f),h.edge,true);
    armor(g.neck,g.waist,(7.1f+breathe*.5f)*bodyWidth,6.1f*bodyWidth,h.suit,h.edge,true);
    const float wide=(1-g.fa)*4.5f;
    const Vec p0=g.at(spine*.97f,-2.f*bodyWidth)-Vec{wide,0}, p1=g.at(spine*.92f,6.5f*bodyWidth)+Vec{wide,0};
    const Vec p2=g.at(spine*.44f,6.f*bodyWidth)+Vec{wide,0}, p3=g.at(spine*.42f,-2.f*bodyWidth)-Vec{wide,0};
    polygon(p0,p1,p2,p3,h.shell.energy(1.04f));
    polygon(mix(p0,p3,.55f),mix(p1,p2,.55f),p2,p3,h.shell.energy(.78f));
    line(p0,p1,1.1f,h.edge.alpha(.8f));
    line(p1,p2,1.2f,ink.alpha(.9f));
    line(p2,p3,1.1f,ink.alpha(.75f));
    line(g.at(spine*.82f,.2f),g.at(spine*.6f,4.8f),1.7f,h.trim.energy(1.2f));
    line(g.at(spine*.72f,.2f),g.at(spine*.52f,4.8f),1.1f,h.trim.energy(.8f));
    const Vec lamp=g.at(spine*.66f,5.4f);
    halo(lamp,6,h.trim.alpha(.28f));
    ellipse(lamp,{1.2f,1.2f},h.trim.energy(2.6f));
    for (int s=0;s<3;++s) line(g.at(spine*(.12f+s*.1f),-3.2f),g.at(spine*(.12f+s*.1f),5.2f),.9f,ink.alpha(.62f));
    // Belt, buckle and pouches.
    polygon(g.at(1.2f,-6.f),g.at(1.2f,6.4f),g.at(4.4f,6.4f),g.at(4.4f,-6.f),h.leather);
    line(g.at(1.2f,-6.f),g.at(1.2f,6.4f),.9f,ink); line(g.at(4.4f,-6.f),g.at(4.4f,6.4f),.9f,ink);
    polygon(g.at(2.3f,4.5f),g.at(2.3f,6.7f),g.at(3.9f,6.7f),g.at(3.9f,4.5f),h.trim.energy(1.1f));
    for (int k=0;k<2;++k) {
        const float v=-.4f+k*3.9f;
        polygon(g.at(1.f,v),g.at(1.f,v+3.4f),g.at(-3.2f,v+3.4f),g.at(-3.2f,v),h.leather.energy(1.15f));
        line(g.at(.2f,v),g.at(.2f,v+3.4f),.7f,ink);
        line(g.at(-3.2f,v),g.at(-3.2f,v+3.4f),.8f,ink);
    }
    if (h.cinematic) {
        // Articulated ceramic breastplate: raised ribs, embossed seams, small
        // fasteners and independent illuminated pressure indicators.
        for (int i=0;i<3;++i) {
            const float u=spine*(.81f-i*.115f);
            const Vec a=g.at(u,.7f), b=g.at(u-.95f,5.0f);
            line(a,b,.68f,h.edge.energy(1.24f).alpha(.83f));
            ellipse(b,{.52f,.52f},h.leather.energy(1.55f));
        }
        line(g.at(spine*.92f,-1.3f),g.at(spine*.46f,-1.2f),.72f,ink.alpha(.72f));
        for (int i=0;i<4;++i) {
            const Vec port=g.at(spine*(.59f-i*.053f),5.6f);
            line(port,port+g.fwd*.9f,.72f,(i==0?h.trim:h.edge).energy(i==0?2.0f:1.f));
        }
        for (int i=0;i<3;++i) {
            const float u=spine*(.26f+i*.075f);
            line(g.at(u,-5.f),g.at(u,-3.2f),.65f,h.edge.alpha(.72f));
        }
    }
    // Unique chest armor profiles: a breacher's reinforced trapezoid, recon's narrow fins,
    // and a specialist's pale split cuirass with an illuminated reactor triangle.
    using Look=Humanoid::Look;
    if (h.look==Look::mercer) {
        polygon(g.at(spine*.95f,-5.6f),g.at(spine*.96f,7.5f),g.at(spine*.38f,7.5f),g.at(spine*.42f,-5.6f),h.trim.energy(.72f));
        polygon(g.at(spine*.87f,-3.8f),g.at(spine*.88f,5.9f),g.at(spine*.53f,5.6f),g.at(spine*.53f,-3.8f),h.shell.energy(.85f));
        for (int i=0;i<3;++i) line(g.at(spine*(.42f+i*.13f),-5.8f),g.at(spine*(.42f+i*.13f),7.6f),1.1f,ink);
        ellipse(g.at(spine*.63f,5.2f),{1.9f,1.5f},h.trim.energy(2.3f));
    } else if (h.look==Look::cole) {
        for (int k=0;k<3;++k) {
            const float u=spine*(.85f-k*.17f);
            line(g.at(u,-4.4f),g.at(u-1.4f,5.3f),1.7f,h.trim.energy(k==0?1.5f:.7f));
        }
        triangle(g.at(spine*.86f,3.9f),g.at(spine*.54f,4.8f),g.at(spine*.61f,1.1f),h.shadow.energy(1.7f));
    } else if (h.look==Look::nyx) {
        polygon(g.at(spine*.94f,-3.2f),g.at(spine*.94f,6.3f),g.at(spine*.43f,5.1f),g.at(spine*.43f,-3.2f),hex(0xc4bfd0).energy(.94f));
        triangle(g.at(spine*.86f,1.2f),g.at(spine*.62f,5.8f),g.at(spine*.51f,-1.3f),h.trim.energy(2.3f));
        for (int k=0;k<3;++k) ellipse(g.at(spine*(.32f+k*.11f),6.1f),{.9f,.8f},h.trim.energy(1.8f));
    }
    // Cross strap.
    if (h.look!=Look::mercer && h.look!=Look::nyx)
    line(g.at(spine*.97f,-1.4f),g.at(spine*.32f,5.8f),2.1f,ink);
    if (h.look!=Look::mercer && h.look!=Look::nyx)
    line(g.at(spine*.97f,-1.4f),g.at(spine*.32f,5.8f),1.3f,h.leather.energy(1.3f));
    ellipse(g.at(spine*.62f,2.4f),{1.4f,1.1f},h.edge);
    // Collar where the neck meets the chest.
    ellipse(g.at(spine*1.02f,1.f),{5.f,3.1f},ink);
    ellipse(g.at(spine*1.02f,1.f),{4.2f,2.4f},h.suit.energy(.75f));
}
} // namespace vl
