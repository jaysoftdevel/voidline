#include "render/renderer.hpp"
#include "render/scene/scene_util.hpp"
#include "game/stairs.hpp"

using namespace vl::scene;

namespace vl {
void Renderer::terrain(const Game& g) {
    material(g);
    const Color tone=metal(g.stage->biome), accent=hex(g.stage->accent), cold=hex(g.stage->cold);
    for (const auto& stair:g.stage->stairs) {
        if (!visible(stair.from.x,stair.to.x-stair.from.x)) continue;
        const auto side=Vec{stair.to.x-stair.from.x,stair.to.y-stair.from.y};
        const Vec a=stair.from+offset,b=stair.to+offset;
        const Vec rise=normal(Vec{-side.y,side.x})*4.f;
        // Steel stringers under grating; amber nosings make the walkable route
        // distinguishable from decorative pipes and distant background ramps.
        line(a+Vec{0,9},b+Vec{0,9},9.f,hex(0x101820));
        line(a+Vec{0,6},b+Vec{0,6},5.f,tone.energy(.62f));
        line(a,b,3.1f,cold.energy(.98f));
        const int count=std::max(2,int(side.x/19.f));
        for (int i=0;i<=count;++i) {
            const float x=mix(stair.from.x,stair.to.x,float(i)/count);
            const float y=mix(stair.from.y,stair.to.y,float(i)/count);
            const Vec at=Vec{x,y}+offset;
            line(at+rise-Vec{0,2},at-rise+Vec{0,9},3.f,tone.energy(.9f));
            line(at-Vec{5,0},at+Vec{7,0},1.5f,accent.energy(1.1f));
        }
        for (int i=0;i<4;++i) {
            const float t=(i+.5f)/4;
            const Vec foot=mix(a,b,t);
            line(foot,foot+Vec{0,31},2.f,tone.energy(.4f));
        }
    }
    for (const auto& surface:g.platforms) {
        if (!surface.ground || !visible(surface.box.x,surface.box.w)) continue;
        const auto& b=surface.box;
        const float left=b.x+offset.x, y=b.y+offset.y;
        const float cycle=std::max(1.f,b.w-135);
        const int seed=int(b.x)+g.index*719;
        const Vec rear=platformRear({left+43+hash(seed+73)*cycle,y},.108f);
        const float x=rear.x;
        const float railBase=rear.y;
        const float span=(60+hash(seed+85)*47)*(1-.108f);
        // Back edge rail, hand-formed pipe and a small warning strip.
        const Color rail=tone.energy(.67f).alpha(.92f);
        for (int j=0;j<3;++j) {
            const float post=x+j*span*.5f;
            rect({post-2,railBase-21,3.1f,21},hex(0x15222c));
            rect({post-1,railBase-19,1.1f,17},rail);
        }
        line({x-4,railBase-22},{x+span+3,railBase-22},3.3f,hex(0x19262f));
        line({x-4,railBase-23},{x+span+3,railBase-23},1.2f,rail);
        line({x-2,railBase-12},{x+span+2,railBase-12},1.2f,rail.energy(.72f));
        const float cy=y+55;
        line({left+4,cy+4},{left+16,cy},3.7f,hex(0x0d161f));
        line({left+16,cy},{left+b.w-14,cy},3.7f,hex(0x0d161f));
        line({left+16,cy-1},{left+b.w-14,cy-1},1.1f,tone.energy(.76f));
        for (int j=0;j<int(b.w/82);++j) {
            const float p=left+32+j*82;
            rect({p,cy-3,3.2f,6},tone.energy(.56f));
        }
    }
    for (size_t index=0;index<g.platforms.size();++index) {
        const auto& surface=g.platforms[index];
        const Rect src=platformBox(g,index);
        if (!visible(src.x,src.w)) continue;
        Rect box{src.x+offset.x,src.y+offset.y,src.w,src.h};
        if (surface.cover) {
            gradient(box,tone.energy(1.12f),hex(0x15232c));
            outline(box,hex(0x101820),2);
            rect({box.x+2,box.y+2,box.w-4,3},cold.energy(1.15f));
            rect({box.x+5,box.y+12,box.w-10,box.h-18},tone.energy(.68f));
            for (int stripe=0;stripe<4;++stripe) {
                const float x=box.x+8+stripe*8;
                line({x,box.y+30},{x+6,box.y+20},3,hex(0xe7ba64).alpha(.82f));
            }
            continue;
        }
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
} // namespace vl
