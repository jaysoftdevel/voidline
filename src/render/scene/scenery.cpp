#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::depthScenery(const Game& g) {
    const Color cold=hex(g.stage->cold), accent=hex(g.stage->accent);
    const float travel=offset.x*.48f+cameraView.tilt.x*12;
    for (int i=0;140+i*154<g.stage->worldWidth*.48f+width;++i) {
        const float x=140+i*154+hash(i+111)*57+travel;
        if (x<-128 || x>width+128) continue;
        const float y=356+hash(i+231)*79+offset.y*.65f+cameraView.tilt.y*4;
        const float h=46+hash(i*9+501)*116;
        const Color steel=hex(0x111e2b).alpha(.38f+hash(i+718)*.19f);
        gradient({x-9,y,18,h},steel.energy(1.14f),steel.energy(.64f));
        line({x-20,y+h*.63f},{x+36,y+h*.24f},3.3f,steel);
        line({x+8,y},{x+8,y+h},1,cold.energy(.44f).alpha(.22f));
        if (i%3!=1) {
            const Vec lamp{x+6,y+h*.33f};
            halo(lamp,15,accent.energy(.95f).alpha(.13f));
            rect({lamp.x-2,lamp.y,4,1.5f},accent.energy(.77f).alpha(.66f));
        }
        if (i%4==0) {
            const float length=115+hash(i+32)*105;
            for (int j=0;j<11;++j) {
                const float a=j/11.f,b=(j+1)/11.f;
                line({x+a*length,y-29+std::sin(a*pi)*23},{x+b*length,y-29+std::sin(b*pi)*23},
                    2.2f,hex(0x131c23).alpha(.58f));
            }
        }
    }
}

void Renderer::foreground(const Game& g) {
    // Sparse foreground steel at the upper edge reads as a nearer plane.
    const Color tone=metal(g.stage->biome), cold=hex(g.stage->cold);
    const float travel=offset.x*1.29f-cameraView.tilt.x*16;
    for (int i=0;85+i*492<g.stage->worldWidth*1.29f+width;++i) {
        const float x=85+i*492+hash(i+941)*160+travel;
        if (x<-230 || x>width+180) continue;
        const float length=100+hash(i+604)*83, y=14+hash(i+619)*26-cameraView.tilt.y*5;
        gradient({x-5,-20,13,y+23},hex(0x101821),hex(0x1a2530));
        line({x,y},{x+length,y-6},7.0f,hex(0x0c161e).alpha(.85f));
        line({x,y-3},{x+length,y-9},1.1f,tone.energy(.75f).alpha(.85f));
        for (int j=0;j<12;++j) {
            const float a=j/12.f,b=(j+1)/12.f;
            line({x+a*length,y+std::sin(a*pi)*16},{x+b*length,y+std::sin(b*pi)*16},
                2.7f,hex(0x0a131a).alpha(.83f));
        }
        if (i%2==0) {
            rect({x+length-14,y-8,10,2},cold.energy(1.40f).alpha(.71f));
            halo({x+length-9,y-7},17,cold.alpha(.23f));
        }
    }
}

void Renderer::scenery(const Game& g) {
    const Color tone=metal(g.stage->biome), accent=hex(g.stage->accent), cold=hex(g.stage->cold);
    for (int i=0;140+i*273<g.stage->worldWidth+215;++i) {
        const float x=140+i*273+hash(i+g.index*32)*90;
        const float y=253+hash(i+54)*126;
        const float w=13+hash(i+17)*27;
        if (!visible(x,80)) continue;
        const Vec p{x+offset.x,y+offset.y};
        gradient({p.x-w*.5f,p.y,w,315},tone.energy(.46f),tone.energy(.13f));
        rect({p.x-w*.5f,p.y,2,295},tone.energy(.69f));
        rect({p.x+w*.24f,p.y,3,295},hex(0x121c26));
        line(p+Vec{-w*.80f,25},p+Vec{w*.57f,77},4,tone.energy(.55f));
        line(p+Vec{w*.65f,69},p+Vec{53,28},4,tone.energy(.37f));
        rect({p.x-22,p.y+1,45,7},hex(0x19232b));
        if (i%2==0) {
            const Vec emitter=p+Vec{11,8};
            rect({emitter.x-7,emitter.y-4,14,5},tone.energy(.94f));
            line(emitter-Vec{5,0},emitter+Vec{5,0},2,accent.energy(1.76f));
            halo(emitter,49,accent.alpha(.28f));
        } else {
            rect({p.x-5,p.y+12,10,54},hex(0x0b1721));
            line(p+Vec{0,17},p+Vec{0,63},2,cold.energy(1.38f));
        }
    }
    for (int i=0;265+i*329<g.stage->worldWidth+215;++i) {
        const float x=265+i*329+hash(i+761)*112;
        if (!visible(x,87)) continue;
        const float y=333+hash(i+73)*81+offset.y;
        const Vec p{x+offset.x,y};
        const Color dark=hex(0x101922);
        if (g.stage->biome==Biome::water) {
            // Condensate on the Tideworks' vertical pipes.
            line(p-Vec{0,49},p+Vec{0,106},9,tone.energy(.46f));
            line(p-Vec{2.1f,49},p+Vec{-2.1f,106},1.25f,cold.energy(.74f).alpha(.67f));
            for (int ring=0;ring<3;++ring) {
                const float cy=p.y+ring*35.f-21;
                rect({p.x-5.9f,cy,11.8f,4.3f},dark);
                rect({p.x-5.9f,cy-.8f,11.8f,1.4f},tone.energy(.68f));
            }
            const float fall=std::fmod(g.time*.83f+hash(i+73),1.f);
            const Vec drop=p+Vec{7.f,fall*150-12};
            line(drop-Vec{0,7},drop,1.2f,cold.energy(1.33f).alpha(.63f*(1-fall)));
        } else if (g.stage->biome==Biome::biodome) {
            // Tendrils split and collect pale bioluminescent buds.
            for (int vine=0;vine<3;++vine) {
                Vec from=p+Vec{vine*13.f,-64.f};
                for (int node=1;node<=6;++node) {
                    const float h=node*16.f;
                    const Vec to=p+Vec{vine*13.f+std::sin(node*.8f+vine)*node*2.3f,h-64.f};
                    line(from,to,2.6f-node*.20f,hex(0x405d45).alpha(.88f));
                    if ((node+vine)%3==1) {
                        const Vec leaf=to+Vec{node%2?-5.f:6.f,-4.f};
                        line(to,leaf,1.5f,accent.energy(.51f));
                        halo(leaf,12,accent.alpha(.18f));
                        ellipse(leaf,{2.4f,1.45f},accent.energy(.83f).alpha(.90f));
                    }
                    from=to;
                }
            }
        } else if (g.stage->biome==Biome::forge) {
            // Hot seams, oil staining and low-speed embers beside the forge ducts.
            gradient({p.x-9,p.y-38,18,108},tone.energy(.53f),dark);
            line(p-Vec{5.f,36},p+Vec{-5.f,47},1.35f,accent.energy(.64f));
            halo(p+Vec{0,12},44,accent.energy(.88f).alpha(.24f));
            for (int c=0;c<4;++c) {
                const float t=std::fmod(g.time*(.34f+c*.07f)+hash(i*17+c),1.f);
                const Vec spark=p+Vec{std::sin(c*9.f+t*4.f)*13,t*-72+22};
                line(spark,spark+Vec{1.5f,-4.5f},1.35f,accent.energy(1.62f).alpha((1-t)*.80f));
            }
        } else {
            const float half=g.stage->biome==Biome::orbital?14.f:9.f;
            const Color paint=g.stage->biome==Biome::orbital?cold:accent;
            gradient({p.x-half,p.y-15,half*2,33},tone.energy(.76f),dark);
            outline({p.x-half,p.y-15,half*2,33},dark,1);
            line({p.x-half+2,p.y-13},{p.x+half-2,p.y-13},1,paint.energy(.47f));
            for (int slot=0;slot<3;++slot)
                rect({p.x-half+3+slot*4.2f,p.y-6,2.3f,1.3f},paint.energy(.90f+hash(i+slot)*.79f));
        }
    }
    for (int i=0;167+511*i<g.stage->worldWidth+215;++i) {
        const float x=167+511*i;
        if (!visible(x,242)) continue;
        const Vec p{x+offset.x,142+hash(i+43)*102+offset.y};
        line(p+Vec{-30,-145},p+Vec{-30,0},2,tone.energy(.4f));
        line(p+Vec{0,-145},p+Vec{0,0},1,tone.energy(.6f));
        line(p-Vec{32,0},p+Vec{20,0},5,tone.energy(.49f));
        line(p-Vec{17,-2},p+Vec{15,2},1.6f,cold.energy(1.41f));
        if (i%3==1) {
            for (int j=0;j<8;++j) {
                const Vec mid=p+Vec{30.f+j*25,35.f+std::sin(j*pi/8)*25};
                const Vec next=p+Vec{55.f+j*25,35.f+std::sin((j+1)*pi/8)*25};
                line(mid,next,2,hex(0x192a34).alpha(.75f));
            }
        }
    }
}
} // namespace vl
