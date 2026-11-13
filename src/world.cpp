#include "renderer.hpp"

namespace vl {
namespace {
float hash(int i) {
    uint32_t n=uint32_t(i)*747796405u+2891336453u;
    n=((n>>((n>>28u)+4u))^n)*277803737u;
    return ((n>>22u)^n)/4294967296.f;
}
Color metal(int sector) {
    constexpr uint32_t tones[]{0x596a72,0x516d75,0x5b6b60,0x80655d,0x586485};
    return hex(tones[sector]);
}
struct GunArt { const char* name; float length,aspect,muzzleV,gripU,gripV,foreU,foreV; };
constexpr GunArt gunArt[]{
    {"carbine",58,.353f,.39f,.29f,.68f,.70f,.56f},
    {"shotgun",62,.314f,.36f,.27f,.76f,.70f,.52f},
    {"arc",70,.231f,.39f,.30f,.80f,.65f,.55f}
};
Color supplyColor(SupplyKind k) {
    return hex(k==SupplyKind::health?0x9dedb4:k==SupplyKind::ammo?0xffc57b:0x89e6ec);
}
}
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
void Renderer::backdrop(const Game& g) {
    const float travel=clamp(-offset.x/(g.stage->worldWidth-width));
    cover(g.stage->art,{0,0,width,height},1.12f,{mix(.24f,.76f,travel),.51f},hex(0xc6d3e4));
    gradient({0,0,width,height},hex(0x091420).alpha(.12f),hex(0x071019).alpha(.65f));
    const Color haze=hex(g.stage->cold);
    for (int i=0;i<12;++i) {
        const float x=std::fmod(260+i*131.3f+offset.x*.072f,1440.f);
        const float y=123+hash(i+60)*255;
        const float pulse=.47f+.24f*std::sin(g.time*1.7f+i*1.32f);
        halo({x,y},19+hash(i)*15,haze.alpha(pulse*.35f));
    }
}
void Renderer::depthScenery(const Game& g) {
    const Color cold=hex(g.stage->cold), accent=hex(g.stage->accent);
    const float travel=offset.x*.48f;
    for (int i=0;i<21;++i) {
        const float x=140+i*154+hash(i+111)*57+travel;
        if (x<-128 || x>width+128) continue;
        const float y=356+hash(i+231)*79+offset.y*.65f;
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
    const Color tone=metal(g.index), cold=hex(g.stage->cold);
    const float travel=offset.x*1.29f;
    for (int i=0;i<11;++i) {
        const float x=85+i*492+hash(i+941)*160+travel;
        if (x<-230 || x>width+180) continue;
        const float length=100+hash(i+604)*83, y=14+hash(i+619)*26;
        gradient({x-5,-6,13,y+9},hex(0x101821),hex(0x1a2530));
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
    const Color tone=metal(g.index), accent=hex(g.stage->accent), cold=hex(g.stage->cold);
    for (int i=0;i<17;++i) {
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
    for (int i=0;i<13;++i) {
        const float x=265+i*329+hash(i+761)*112;
        if (!visible(x,87)) continue;
        const float y=333+hash(i+73)*81+offset.y;
        const Vec p{x+offset.x,y};
        const Color dark=hex(0x101922);
        if (g.index==1) {
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
        } else if (g.index==2) {
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
        } else if (g.index==3) {
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
            const float half=g.index==4?14.f:9.f;
            const Color paint=g.index==4?cold:accent;
            gradient({p.x-half,p.y-15,half*2,33},tone.energy(.76f),dark);
            outline({p.x-half,p.y-15,half*2,33},dark,1);
            line({p.x-half+2,p.y-13},{p.x+half-2,p.y-13},1,paint.energy(.47f));
            for (int slot=0;slot<3;++slot)
                rect({p.x-half+3+slot*4.2f,p.y-6,2.3f,1.3f},paint.energy(.90f+hash(i+slot)*.79f));
        }
    }
    for (int i=0;i<9;++i) {
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
void Renderer::terrain(const Game& g) {
    material(g);
    const Color tone=metal(g.index), accent=hex(g.stage->accent), cold=hex(g.stage->cold);
    for (const auto& surface:g.platforms) {
        if (!surface.ground || !visible(surface.box.x,surface.box.w)) continue;
        const auto& b=surface.box;
        const float left=b.x+offset.x, y=b.y+offset.y;
        const float cycle=std::max(1.f,b.w-135);
        const int seed=int(b.x)+g.index*719;
        const float x=left+43+hash(seed+73)*cycle;
        const float span=60+hash(seed+85)*47;
        // Back edge rail, hand-formed pipe and a small warning strip.
        const Color rail=tone.energy(.67f).alpha(.92f);
        for (int j=0;j<3;++j) {
            const float post=x+j*span*.5f;
            rect({post-2,y-21,3.1f,21},hex(0x15222c));
            rect({post-1,y-19,1.1f,17},rail);
        }
        line({x-4,y-22},{x+span+3,y-22},3.3f,hex(0x19262f));
        line({x-4,y-23},{x+span+3,y-23},1.2f,rail);
        line({x-2,y-12},{x+span+2,y-12},1.2f,rail.energy(.72f));
        const float cy=y+55;
        line({left+4,cy+4},{left+16,cy},3.7f,hex(0x0d161f));
        line({left+16,cy},{left+b.w-14,cy},3.7f,hex(0x0d161f));
        line({left+16,cy-1},{left+b.w-14,cy-1},1.1f,tone.energy(.76f));
        for (int j=0;j<int(b.w/82);++j) {
            const float p=left+32+j*82;
            rect({p,cy-3,3.2f,6},tone.energy(.56f));
        }
    }
    for (const auto& surface:g.platforms) {
        const Rect src=surface.box;
        if (!visible(src.x,src.w)) continue;
        Rect box{src.x+offset.x,src.y+offset.y,src.w,src.h};
        const float lip=surface.ground?19.f:6.f;
        gradient(box,tone.energy(.55f).alpha(.04f),hex(0x0b141e).alpha(.48f));
        rect({box.x,box.y,box.w,1.2f},cold.energy(.84f).alpha(.91f));
        rect({box.x,box.y+2,box.w,1.6f},tone.energy(.47f).alpha(.70f));
        rect({box.x,box.y+lip,box.w,2},hex(0x0b151e));
        rect({box.x,box.y,2,box.h},tone.energy(.56f));
        rect({box.right()-2,box.y,2,box.h},tone.energy(.26f));
        const int panels=std::max(1,int(src.w/(surface.ground?62:36)));
        for (int i=0;i<panels;++i) {
            const float cell=box.w/panels, x=box.x+i*cell;
            const float pattern=hash(int(src.x)+i*13);
            if (x+cell<0 || x>width) continue;
            const Rect plate{x+3,box.y+lip+4,cell-6,surface.ground?36.f:6.f};
            gradient(plate,tone.energy(.53f+pattern*.21f).alpha(.58f),tone.energy(.22f+pattern*.25f).alpha(.65f));
            if (surface.ground) {
                line({plate.x+2,plate.y+4},{plate.x+11,plate.y+4},1.3f,tone.energy(1.06f).alpha(.61f));
                rect({plate.x+1,plate.y+7,2,plate.h-10},hex(0x18232d).alpha(.59f));
                for (int bolt=0;bolt<2;++bolt) {
                    const float bx=bolt==0?plate.x+4:plate.right()-4;
                    ellipse({bx,plate.y+plate.h-4},{1.25f,1.15f},hex(0x121a22));
                    rect({bx-.6f,plate.y+plate.h-5,1.2f,.5f},tone.energy(1.17f));
                }
                if (pattern>.74f) {
                    for (int streak=0;streak<4;++streak) {
                        const float bx=plate.x+7+hash(i*28+streak+int(src.x))*std::max(1.f,plate.w-13);
                        line({bx,plate.y+3},{bx+2.3f,plate.y+10+hash(streak+i*9)*17},1.1f,
                            accent.energy(.61f).alpha(.36f));
                    }
                }
            }
            rect({plate.x,plate.y,plate.w,1},tone.energy(.74f));
            rect({x,box.y+2,1,lip-3},hex(0x101923));
            rect({x+cell-7,box.y+3,2,2},hex(0x0a1017));
            if (surface.ground) {
                line({plate.x+5,plate.y+23},{plate.right()-7,plate.y+23},1,tone.energy(.22f));
                for (int s=0;s<5;++s) {
                    const float slot=x+9+s*6;
                    line({slot,box.y+12},{slot+3,box.y+6},1.5f,tone.energy(.41f));
                }
                const Rect frame{x+5,box.y+65,cell-10,21+pattern*15};
                rect(frame,hex(0x0f1820).alpha(.76f));
                rect({frame.x,frame.y,frame.w,1},tone.energy(.45f));
                line({frame.x,frame.bottom()},{frame.right(),frame.y},2,tone.energy(.42f));
            }
            if (i%3==1 || surface.lift) {
                const float y=box.y+(surface.ground?36:10);
                rect({x+8,y-2,16,5},hex(0x101a25));
                line({x+10,y},{x+20,y},1.65f,(surface.lift?accent:cold).energy(surface.lift?1.94f:1.39f));
            }
        }
        if (surface.ground) for (float k=4;k<38;k+=9) {
            line({box.x+k,box.y+14},{box.x+k+8,box.y+3},2.3f,accent.energy(.76f));
            line({box.right()-k-9,box.y+14},{box.right()-k-1,box.y+3},2.3f,accent.energy(.76f));
        }
    }
}
void Renderer::objects(const Game& g) {
    const Color cold=hex(g.stage->cold),tone=metal(g.index);
    for (const auto& vent:g.stage->vents) {
        if (!visible(vent.x)) continue;
        const Vec p{vent.x+offset.x,vent.y+offset.y};
        const int state=g.ventState(vent.phase);
        const Color hazard=hex(g.index==2?0xbfff92:g.index==4?0xbd9aff:0xff9b62);
        rect({p.x-21,p.y-4,42,4},tone.energy(.68f));
        for (int i=0;i<7;++i) rect({p.x-17+i*5,p.y-5,2,3},hazard.energy(state?1.8f:.51f));
        if (state) halo(p-Vec{0,14},42,hazard.alpha(state==2?.48f:.29f));
        if (state==2) for (int i=0;i<11;++i) {
            const float t=std::fmod(g.time*(.82f+hash(i)*.61f)+i*.111f,1.f);
            const Vec start=p+Vec{std::sin(i*2.3f+t*5)*(7+t*14),-t*70};
            const Color c=hazard.energy(1.5f).alpha((1-t)*.71f);
            line(start,start+Vec{std::sin(i*4.f)*8,-8-t*14},2.2f,c);
        }
    }
    const auto beacon=g.stage->beacon+offset;
    if (visible(g.stage->beacon.x)) {
        const Color status=g.checkpoint?hex(0x92ffbe):cold;
        rect({beacon.x-22,beacon.y-4,44,4},tone.energy(.80f));
        rect({beacon.x-11,beacon.y-46,22,44},hex(0x16232b));
        outline({beacon.x-11,beacon.y-46,22,44},tone.energy(.94f));
        rect({beacon.x-2,beacon.y-38,4,28},status.energy(1.67f));
        halo(beacon-Vec{0,41},32,status.alpha(.51f));
        ellipse(beacon-Vec{0,47},{6,3},status.energy(2.f));
    }
    const auto exit=g.stage->exit+offset;
    if (visible(g.stage->exit.x)) {
        const Color status=g.bossDead?hex(0x85ffd4):hex(0xff694f);
        gradient({exit.x-45,exit.y-109,90,109},tone.energy(.8f),tone.energy(.42f));
        rect({exit.x-32,exit.y-90,64,90},hex(0x101b26));
        rect({exit.x-29,exit.y-84,58,84},hex(0x06121b));
        for (int i=0;i<8;++i) {
            rect({exit.x-29,exit.y-82+i*9.f,58,2},tone.energy(.38f));
            if (g.bossDead) rect({exit.x-26,exit.y-81+i*9.f,52,1},status.alpha(.33f));
        }
        rect({exit.x-32,exit.y-89,3,85},status.energy(1.8f));
        rect({exit.x+29,exit.y-89,3,85},status.energy(1.8f));
        rect({exit.x-44,exit.y-108,88,13},hex(0x15202a));
        halo(exit-Vec{0,84},58,status.alpha(.43f));
        text({exit.x-32,exit.y-98},g.bossDead?"EXTRACTION":"LOCKED",8,status.energy(1.55f),true);
    }
    for (size_t i=0;i<g.supplies.size();++i) {
        const auto& s=g.supplies[i];
        if (s.used || !visible(s.p.x)) continue;
        const Color c=supplyColor(s.kind);
        const Vec p=s.p+offset+Vec{0,std::sin(g.time*4+float(i)) * 2};
        halo(p,26,c.alpha(.44f));
        gradient({p.x-11,p.y-9,22,17},hex(0x465761),hex(0x15222c));
        outline({p.x-11,p.y-9,22,17},c.energy(.77f));
        rect({p.x-8,p.y-6,2,11},c.energy(1.49f));
        rect({p.x+6,p.y-6,2,11},c.energy(1.49f));
        if (s.kind==SupplyKind::health) {
            rect({p.x-1,p.y-5,2,9},c.energy(1.8f));
            rect({p.x-4,p.y-2,8,2},c.energy(1.8f));
        } else if (s.kind==SupplyKind::ammo) {
            for (int j=-3;j<=3;j+=3) rect({p.x+j,p.y-4,1.5f,7},c.energy(1.68f));
        } else {
            ellipse(p,{4,4},c.energy(1.64f));
        }
    }
    auto projectile=[&](const Shot& s,bool friendly) {
        if (!visible(s.p.x)) return;
        const Vec p=s.p+offset, dir=normal(s.v);
        const float tail=s.longShot?34.f:s.wave?23.f:friendly?16.f:11.f;
        halo(p,s.longShot?22:14,s.color.energy(1.40f).alpha(.48f));
        line(p-dir*tail,p+dir*3,s.wave?6.3f:3.9f,s.color.energy(s.longShot?2.6f:1.91f).alpha(.78f));
        line(p-dir*tail*.58f,p+dir*3,s.wave?2.6f:1.5f,hex(0xfff5e4).energy(2.2f));
    };
    for (const auto& s:g.shots) projectile(s,true);
    for (const auto& s:g.hostile) projectile(s,false);
}
// Actors are drawn after lighting and fog so their flat, saturated shapes stay crisp against the backdrop.
void Renderer::actors(const Game& g) {
    const Color accent=hex(g.stage->accent);
    for (const auto& e:g.enemies) if (e.alive && visible(e.box.x,e.box.w)) drawEnemy(e,accent,g.index);
    drawPlayer(g);
    drawRings(g);
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
void Renderer::drawPlayer(const Game& g) {
    const auto& p=g.player;
    const Rect box=p.box;
    const Vec shift=offset+playerShift;
    const Vec ground{box.center().x+shift.x,box.bottom()+shift.y};
    const Color trim=hex(g.character->color);
    const bool hurtBlink=p.flash>0 && std::sin(p.flash*80)>0;
    // A dark flat suit with saturated armor shells reads clearly against the painted backdrops.
    const Color suit=hurtBlink?hex(0xd9a79a):mix(hex(0x465466),trim,.18f);
    const Color shell=hurtBlink?hex(0xffffff):mix(trim,hex(0xffffff),.06f).energy(1.12f);
    const Color plate=suit;
    const Color edge=mix(trim,hex(0xffffff),.7f), shadow=hex(0x232d3a), seams=hex(0x0a0e15);
    const Color ink=hex(0x06080d);
    const float bend=std::min(8.f,pose.compression);
    Vec hip{ground.x+pose.lean*(9+pose.crouch*5),ground.y-28+pose.pelvis+bend+pose.crouch*12.5f};
    for (const auto& heel:drawHeels) {
        const float x=std::min(30.f,std::abs(hip.x-heel.x-offset.x));
        const float reach=std::sqrt(32.f*32.f-x*x);
        hip.y=std::max(hip.y,heel.y+offset.y-3.f-reach);
    }
    if (p.support>=0) {
        halo(ground-Vec{0,1},24,hex(0x090f19).alpha(.50f));
        ellipse(ground,{20.f+pose.crouch*4,2.8f},hex(0x090f16).alpha(.56f));
    }
    drawTrails(g,ground);
    if (p.flip>0) {
        const float t=1-p.flip/flipSeconds;
        beginSpin({ground.x,ground.y-26+pose.crouch*8},p.flipDirection*2*pi*smooth(t));
    }
    auto drawLeg=[&](int i,bool far) {
        const Vec contact=drawHeels[i]+offset;
        const int dir=pose.direction;
        const Vec toeRay=ray(drawFootAngles[i]);
        const Vec forward{toeRay.x*dir,-toeRay.y};
        const Vec footEnd=contact+forward*8;
        const float lift=p.support>=0?std::max(0.f,std::max(contact.y,footEnd.y)-ground.y):0.f;
        const Vec heel=contact-Vec{0,2.4f+lift};
        const Vec toe=footEnd-Vec{0,2.4f+lift};
        const Vec ankle=heel-Vec{0,2.1f};
        const Vec socket=hip+Vec{i==0?-2.f:2.f,i==0?-1.f:.5f};
        const Vec knee=solveKnee(socket,ankle,17.8f,16.5f,dir);
        const Color tint=far?shadow:plate;
        armor(socket,knee,far?3.0f:4.0f,far?2.1f:3.0f,tint,edge,!far);
        armor(knee,ankle,far?2.45f:3.25f,far?1.85f:2.55f,tint.energy(.85f),edge,!far);
        ellipse(knee,{far?2.4f:3.4f,far?2.7f:3.7f},seams);
        if (!far) {
            line(knee+Vec{-1.8f,-.8f},knee+Vec{1.9f,.8f},1.6f,trim.energy(1.20f));
            line(mix(knee,ankle,.29f),mix(knee,ankle,.65f),1.2f,hex(0xc1cdd3).alpha(.56f));
        }
        line(heel-Vec{2.2f*dir,0},toe,4.6f,hex(0x111a23));
        line(heel-Vec{1.6f*dir,1.05f},toe-Vec{0,.82f},2.2f,tint);
        line(heel-Vec{1.7f*dir,-1.35f},toe+Vec{0,1.35f},.85f,seams);
    };
    const int rear=drawHeels[0].x*pose.direction<drawHeels[1].x*pose.direction?0:1;
    drawLeg(rear,true); drawLeg(1-rear,false);

    const int facing=p.facing;
    const float counter=std::sin(pose.cycle*2*pi)*pose.weight;
    const float tilt=pose.lean*(29-4*pose.crouch);
    const Vec neck=hip+Vec{tilt+facing*(2.4f+pose.crouch*5.8f),-18.0f+pose.crouch*7.0f+pose.tuck*.65f};
    {
        // Scarf: a short ribbon whose drift follows velocity, in the operative's color.
        const Vec base=neck+Vec{-2.5f*facing,2.f};
        const Vec wind{-clamp(p.velocity.x/420,-1.3f,1.3f)*4.6f-facing*2.4f,-clamp(p.velocity.y/760,-1.f,1.f)*3.2f};
        Vec from=base;
        for (int i=1;i<=7;++i) {
            const Vec to=base+Vec{wind.x*i*1.1f,i*1.05f+wind.y*i*.8f+std::sin(g.time*11-i*.85f)*i*.38f};
            const float width=std::max(1.4f,5.6f-i*.55f);
            line(from,to,width+2.6f,ink);
            from=to;
        }
        from=base;
        for (int i=1;i<=7;++i) {
            const Vec to=base+Vec{wind.x*i*1.1f,i*1.05f+wind.y*i*.8f+std::sin(g.time*11-i*.85f)*i*.38f};
            const float width=std::max(1.4f,5.6f-i*.55f);
            line(from,to,width,trim.energy(i%2?1.f:.82f));
            from=to;
        }
    }
    const Vec back=neck-Vec{7.8f*facing,-4.3f};
    const Vec packBottom=hip-Vec{6.6f*facing,3.8f};
    armor(back,packBottom,4.7f,4.2f,shadow,edge.energy(.68f),true);
    line(back-Vec{3.f*facing,-1.4f},packBottom-Vec{3.f*facing,0},1.15f,trim.energy(1.02f));
    armor(neck,hip,7.2f,5.7f,shell,edge,true);
    const Vec chest=mix(neck,hip,.42f);
    line(chest-Vec{4.2f,1.8f},chest+Vec{4.8f,-1.0f},3.4f,trim.energy(.96f));
    line(chest-Vec{4.2f,2.7f},chest+Vec{4.8f,-2.1f},.95f,edge.alpha(.76f));
    line(hip+Vec{-5.8f,.65f},hip+Vec{5.6f,1.9f},3.1f,seams);
    line(hip+Vec{-3.1f,1.f},hip+Vec{2.f,1.f},.85f,edge.energy(.76f));

    const Vec head=neck+Vec{facing*(2.2f+pose.crouch*1.8f),-6.75f};
    const Vec crown=head+Vec{0,-2};
    ellipse(head,{8.6f,9.f},hex(0x111b25));
    ellipse(crown,{7.35f,6.55f},shell.energy(1.05f));
    triangle(head+Vec{3.f*facing,-3},head+Vec{8.f*facing,1.2f},head+Vec{5.2f*facing,6.7f},shell.energy(.7f));
    line(head+Vec{-4.4f,-5.6f},head+Vec{3.7f,-7.f},1.25f,edge);
    const Vec visor=head+Vec{3.7f*facing,-.75f};
    line(visor-Vec{4.55f,0},visor+Vec{4.55f,0},3.5f,seams);
    line(visor-Vec{3.45f,.35f},visor+Vec{3.45f,-.28f},1.2f,trim.energy(2.25f));
    ellipse(head-Vec{4.3f*facing,-2.0f},{1.6f,1.75f},seams);
    halo(visor,9,trim.alpha(.34f));
    const Vec shoulderRaw=neck+Vec{-1.f*facing-counter*(2.2f+pose.run)*pose.direction,2.6f};
    const Vec spunShoulder=transform(shoulderRaw);

    const float forwardFacing=p.facing>0?1.f:-1.f;
    const float local=std::atan2(std::sin(p.aim),std::abs(std::cos(p.aim)));
    const float barrel=mix(local,.53f+counter*.13f,carry);
    const Vec aim{std::cos(barrel)*forwardFacing,std::sin(barrel)};
    const Vec side{-aim.y,aim.x};
    const Vec up=forwardFacing>0?Vec{aim.y,-aim.x}:Vec{-aim.y,aim.x};
    const float recoil=p.muzzle>0?3.7f*p.muzzle/.069f:0;
    const Vec mount=g.weaponPivot()+shift+Vec{-carry*5.f*facing,(pose.crouch-p.gait.crouch)*12+carry*7.f};
    const Vec muzzle=(carry>0?mount+aim*muzzleReach[g.gun]:g.muzzle()+shift)-aim*(recoil*.45f);
    const bool art=g.gun<3;
    const GunArt& model=gunArt[art?g.gun:0];
    const float gunLength=model.length,gunHeight=model.length*model.aspect;
    const Vec axisU=aim*gunLength,axisV=up*(-gunHeight);
    const Vec gripPoint=muzzle+axisU*(model.gripU-1)+axisV*(model.gripV-model.muzzleV);
    const Vec forePoint=muzzle+axisU*(model.foreU-1)+axisV*(model.foreV-model.muzzleV);
    const Vec hand=art?gripPoint:mount+aim*14-aim*recoil;
    const Vec farHand=art?forePoint:mount+aim*22+side*(2.3f*facing);
    const Vec shoulder=spunShoulder;
    endSpin();
    const Vec elbow=mix(shoulder,hand,.55f)+Vec{-3.1f*facing,5.2f+pose.crouch*2.3f+pose.weight};
    armor(shoulder+Vec{-2.f,2},farHand,2.75f,1.9f,suit.energy(.8f),edge.energy(.54f),false);
    ellipse(shoulder,{5.05f,4.85f},ink);
    const Color gunColor=hex(weapons[g.gun].color);
    if (art) {
        sprite(model.name,muzzle,axisU,axisV,{1.f,model.muzzleV});
    } else {
        const Vec receiver=hand-aim*7.3f;
        const Vec fore=muzzle-aim*3.5f;
        armor(receiver,fore,2.25f,1.65f,hex(0x475965),hex(0xb1bec0),true);
        line(fore,muzzle,2.55f,hex(0x96a8b3));
        line(receiver+aim*2-side*(1.75f*facing),fore-side*(1.75f*facing),.95f,gunColor.energy(1.41f));
        line(hand,hand+side*(5.2f*facing),2.6f,hex(0x263640));
    }
    armor(shoulder,elbow,4.5f,2.3f,shell,edge,true);
    armor(elbow,hand,2.95f,2.15f,suit.energy(1.1f),edge,true);
    ellipse(hand,{2.9f,2.7f},ink);
    ellipse(hand,{2.1f,2.f},suit.energy(1.3f));
    if (art) ellipse(farHand,{2.5f,2.3f},suit.energy(1.2f));
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
void Renderer::drawEnemy(const Enemy& e,Color accent,int sector) {
    const Rect b=e.box;
    const Vec center=b.center()+offset;
    const float x=center.x,y=center.y;
    const int facing=e.facing;
    const Color alert=hex(0xff6870);
    const Color rim=e.hit>0?hex(0xffffff):mix(hex(0x9fb0b8),accent,.35f);
    const Color hull=e.hit>0?hex(0xfff0d8):mix(hex(0x39424e),accent,.24f);
    const Color black=hex(0x06080d);
    const float pace=clamp(std::abs(e.vx)/96);
    if (e.kind==EnemyKind::drone) {
        line({x-23,y-2},{x+23,y-2},4,hull);
        line({x-17,y-9},{x+17,y-9},2,rim);
        for (float side:{-1.f,1.f}) {
            const Vec rotor{x+side*20,y+6};
            ellipse(rotor,{6,4},black);
            halo(rotor+Vec{0,7},18,accent.alpha(.41f));
            line(rotor,rotor+Vec{0,8},2.5f,accent.energy(1.7f));
        }
        ellipse(center,{16,11},black); ellipse(center-Vec{0,1},{13,8.5f},hull);
        emissive(center+Vec{facing*6.f,-.5f},3.1f,alert,2);
    } else if (e.kind==EnemyKind::turret) {
        const Vec base{x,b.bottom()+offset.y};
        triangle(base-Vec{23,0},center-Vec{0,15},base+Vec{23,0},hull);
        ellipse(center,{15,12},black);
        ellipse(center-Vec{0,1},{12,9},hull);
        const Vec gun=ray(e.warning>0?e.aim:(facing>0?0.f:pi));
        limb(center,center+gun*23,7,hull,rim);
        emissive(center,3.6f,alert,e.warning>0?3.1f:1.4f);
    } else if (e.kind==EnemyKind::stalker) {
        for (int i=0;i<4;++i) {
            const float d=(i%2?1.f:-1.f), z=i<2?1.f:-1.f;
            const float t=std::sin(e.distance*.17f+i*pi*.5f)*5*pace;
            const Vec joint{x+z*9,y+2}, knee{x+z*17+t,y+4+d*4};
            const Vec toe{x+z*24-t,y+b.h*.49f-std::abs(t)*.5f};
            limb(joint,knee,4,hull,rim); limb(knee,toe,3,hull,rim.energy(.74f));
        }
        ellipse(center-Vec{0,1},{17,9},black);
        ellipse(center-Vec{0,2},{15,7},hull);
        emissive(center+Vec{facing*12.f,-2},3,alert,1.9f);
    } else if (e.kind==EnemyKind::ranger) {
        const Vec pelvis{x,y+4}, shoulders{x+facing*3.f,y-14};
        for (int i=0;i<2;++i) {
            const float cycle=std::fmod(e.distance/56+.5f*i,1.f);
            const float xFoot=(cycle<.61f?mix(15,-14,cycle/.61f):mix(-14,15,smooth((cycle-.61f)/.39f)))*pace*facing;
            const float rise=cycle<.61f?0.f:std::sin(pi*(cycle-.61f)/.39f)*7*pace;
            const Vec foot{x+xFoot+(i==0?-3.f:3.f),y+b.h*.5f-rise-3};
            const Vec knee=solveKnee(pelvis,foot,13,13,facing);
            limb(pelvis,knee,6,hull.energy(i?.97f:.68f),rim);
            limb(knee,foot,5,hull.energy(i?.87f:.61f),rim);
            line(foot-Vec{2,0},foot+Vec{4.f*facing,0},3,black);
        }
        limb(pelvis,shoulders,13,hull,rim);
        const Vec head=shoulders-Vec{0,8};
        ellipse(head,{7.5f,7.5f},black);
        ellipse(head-Vec{0,1},{6,6},hull);
        line(head-Vec{4,1},head+Vec{4,-1},1.7f,alert.energy(2.25f));
        const Vec gun=ray(e.warning>0?e.aim:(facing>0?.1f:pi-.1f));
        limb(shoulders+Vec{0,3},shoulders+gun*20+Vec{0,8},5.8f,hull,rim);
        limb(shoulders+gun*12+Vec{0,7},shoulders+gun*26+Vec{0,7},4.3f,black,rim);
    } else {
        const Vec base{x,y+b.h*.5f};
        const Color core=sector==4?hex(0xa6c8ff):accent;
        const float gait=std::sin(e.distance*.084f)*7*pace;
        if (sector!=4) for (float side:{-1.f,1.f}) {
            const Vec hip{x+side*24,y+11}, knee{x+side*(34+gait),y+29}, foot{base.x+side*(38+gait),base.y-2};
            limb(hip,knee,15,hull,rim); limb(knee,foot,12,hull.energy(.76f),rim);
            line(foot-Vec{9,0},foot+Vec{10,0},5,black);
        }
        switch (sector) {
        case 0:
            gradient({x-37,y-32,74,63},hull.energy(1.3f),hull.energy(.54f));
            outline({x-37,y-32,74,63},black,2.5f);
            for (float side:{-1.f,1.f}) {
                limb(center+Vec{side*31,-20},center+Vec{side*48,17},12,hull,rim);
                line(center+Vec{side*48,17},center+Vec{side*51,28},5,black);
            }
            for (int i=0;i<4;++i) rect({x-29+i*5.f,y-23,2,11},core.energy(1.23f));
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
            break;
        case 2:
            ellipse(center,{39,37},black);
            for (int i=0;i<9;++i) {
                const Vec d=ray(i*2*pi/9), side{-d.y,d.x};
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
            break;
        case 4:
            ellipse(center,{55,34},black);
            ellipse(center-Vec{0,3},{51,30},hull);
            line({x-63,y-4},{x+63,y-4},4,rim);
            for (float i:{-1.f,1.f}) line({x+i*31,y+21},{x+i*33,y+42},4,core.energy(1.85f));
            break;
        }
        const Vec dir=ray(e.warning>0?e.aim:facing>0?0:pi);
        limb(center+Vec{facing*29.f,-4},center+dir*56+Vec{0,-4},sector==3?14.f:9.f,hull,rim);
        ellipse(center,{18,18},black);
        ellipse(center,{13,13},core.energy(.58f));
        emissive(center,8.7f,core,e.warning>0?3.5f:2.15f);
        ellipse(center,{3.3f,3.3f},hex(0xfff6df).energy(2.2f));
    }
    if (e.warning>0) {
        const Vec start{x,y-3}, dir=ray(e.aim);
        line(start,start+dir*99,1.1f,alert.energy(1.4f).alpha(.34f));
        halo(start,26,alert.energy(1.5f).alpha(.54f));
    }
    if (e.kind!=EnemyKind::boss && e.hp<e.maxHp) {
        const Rect track{x-14,y-b.h*.5f-7,28,2.5f};
        rect(track,black);
        rect({track.x,track.y,track.w*clamp(float(e.hp)/e.maxHp),track.h},alert);
    }
}
void Renderer::drawParticles(const Game& g) {
    const Color cold=hex(g.stage->cold),accent=hex(g.stage->accent);
    for (int i=0;i<66;++i) {
        const float depth=hash(i+190);
        float x=std::fmod(hash(i*7+14)*1320+g.time*(6+depth*17)+offset.x*(.08f+depth*.13f),1320.f);
        if (x<0) x+=1320;
        const float y=120+hash(i+80)*475+std::sin(g.time*(.5f+depth)+i)*15;
        const float glow=.16f+depth*.26f;
        const Vec p{x-60,y};
        const Color tint=g.index==3?accent:cold;
        if (i%4==0) halo(p,3+depth*4,tint.alpha(glow));
        rect({p.x,p.y,.6f+depth*1.9f,.7f+depth*1.6f},tint.energy(1.23f).alpha(glow));
    }
    for (const auto& particle:g.particles) {
        if (!visible(particle.p.x)) continue;
        const float life=particle.life/particle.maxLife;
        const Vec p=particle.p+offset;
        const Color tint=particle.color;
        if (particle.kind==ParticleKind::smoke) {
            halo(p,particle.size*(4+9*(1-life)),tint.alpha(life*.22f));
        } else if (particle.kind==ParticleKind::casing) {
            const Vec d=ray(particle.angle)*2.5f;
            line(p-d,p+d,2.6f,hex(0x1a1208).alpha(clamp(particle.life*3)));
            line(p-d,p+d,1.5f,tint.energy(1.25f).alpha(clamp(particle.life*3)));
        } else if (particle.kind==ParticleKind::chip) {
            const Vec d=ray(particle.angle)*particle.size;
            const Vec o{-d.y*.50f,d.x*.50f};
            triangle(p+d,p-d+o,p-d-o,tint.energy(.88f+life*.67f).alpha(life));
        } else {
            const Vec v=normal(particle.v)*particle.size*(1.9f+life*2);
            line(p-v,p,particle.size*.72f,tint.energy(1.55f+life*1.51f).alpha(clamp(life*2)));
        }
    }
}
void Renderer::drawRings(const Game& g) {
    for (const auto& r:g.rings) {
        if (!visible(r.p.x,r.radius)) continue;
        const float t=1-r.life/r.maxLife;
        const float radius=r.radius*(1-(1-t)*(1-t));
        const float alpha=(1-t)*(1-t);
        const Vec c=r.p+offset;
        constexpr int segments=28;
        for (int i=0;i<segments;++i) {
            const Vec a=c+ray(i*2*pi/segments)*radius, b=c+ray((i+1)*2*pi/segments)*radius;
            line(a,b,1.5f+7.f*(1-t),r.color.energy(2.f).alpha(alpha*.85f));
            line(a,b,1.f+2.f*(1-t),hex(0xffffff).alpha(alpha));
        }
        halo(c,radius*1.25f,r.color.energy(1.6f).alpha(alpha*.35f));
    }
}
void Renderer::drawTrails(const Game& g,Vec ground) {
    const auto& p=g.player;
    const float speed=std::abs(p.velocity.x);
    const float slow=clamp((1-g.timeScale)/.72f);
    const bool flipping=p.flip>0;
    const float intensity=std::max(clamp((speed-300)/220),flipping?1.f:0.f);
    if (intensity<=.02f && slow<.1f) return;
    const Color tint=hex(g.character->color);
    const Vec travel=length(p.velocity)>1?normal(p.velocity):Vec{float(p.facing),0};
    const Vec center=ground-Vec{0,26};
    const Vec perpendicular{-travel.y,travel.x};
    for (int i=0;i<4;++i) {
        const float lane=(i-1.5f)*9.f;
        const float streak=18+hash(i+int(g.time*30))*26+intensity*34+slow*20;
        const Vec start=center+perpendicular*lane-travel*(6+hash(i+9)*8);
        line(start,start-travel*streak,1.6f+hash(i+40)*1.4f,tint.energy(1.5f).alpha((.12f+.26f*std::max(intensity,slow))*(1-std::abs(lane)/24)));
    }
}
void Renderer::collectLights(const Game& g) {
    lights.clear(); blockers.clear();
    const Color cold=hex(g.stage->cold), accent=hex(g.stage->accent);
    for (const auto& p:g.platforms) if (visible(p.box.x,p.box.w))
        blockers.push_back({p.box.x+offset.x,p.box.y+offset.y,p.box.w,p.box.h});
    for (const auto& e:g.enemies) {
        if (!e.alive || !visible(e.box.x,e.box.w)) continue;
        const auto b=e.box;
        if (e.kind==EnemyKind::boss || e.kind==EnemyKind::turret)
            blockers.push_back({b.x+offset.x+b.w*.18f,b.y+offset.y+b.h*.20f,b.w*.64f,b.h*.70f});
    }
    const auto& p=g.player.box;
    blockers.push_back({p.x+offset.x+8,p.y+offset.y+5,16,p.h-13});
    if (g.player.muzzle>0)
        lights.push_back({g.muzzle()+offset+playerShift,178,4.8f*g.player.muzzle/.069f,hex(weapons[g.gun].color),ray(g.player.aim),-.1f,true});
    for (const auto& e:g.enemies) if (e.alive && visible(e.box.x,e.box.w) && (e.kind==EnemyKind::boss || e.warning>0))
        lights.push_back({e.box.center()+offset,110,e.warning>0?2.16f:1.35f,accent});
    lights.push_back({{p.center().x+offset.x,p.y+offset.y+std::min(p.h*.31f,16.f)},128,1.05f,hex(g.character->color),ray(g.player.aim),.45f,false});
    if (visible(g.stage->beacon.x)) lights.push_back({g.stage->beacon+offset-Vec{0,47},102,1.28f,g.checkpoint?hex(0x96ffc3):cold});
    if (visible(g.stage->exit.x)) lights.push_back({g.stage->exit+offset-Vec{0,86},165,1.65f,g.bossDead?hex(0x8effc8):hex(0xff7250)});
    for (const auto& vent:g.stage->vents) if (visible(vent.x) && g.ventState(vent.phase)>0)
        lights.push_back({Vec{vent.x,vent.y-7}+offset,98,1.78f,accent,{0,-1},.12f});
    for (int i=0;i<17;i+=2) {
        const float x=140+i*273+hash(i+g.index*32)*90;
        if (!visible(x)) continue;
        const float y=253+hash(i+54)*126;
        lights.push_back({Vec{x+11,y+8}+offset,215,1.86f+.17f*std::sin(g.time*2+i),accent,{.12f,1},.16f});
    }
    for (int i=0;i<9;++i) {
        const float x=167+511*i;
        if (!visible(x)) continue;
        lights.push_back({Vec{x,143+hash(i+43)*102}+offset,301,1.49f,cold,{-.19f,1},.27f});
    }
    for (const auto& s:g.shots) if (s.longShot && visible(s.p.x)) lights.push_back({s.p+offset,58,.88f,s.color,{0,1},-1,false});
    for (const auto& s:g.hostile) if (visible(s.p.x)) lights.push_back({s.p+offset,36,.65f,s.color,{0,1},-1,false});
}
}
