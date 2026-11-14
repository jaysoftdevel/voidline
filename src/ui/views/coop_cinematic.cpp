#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/coop_story.hpp"

using namespace vl::ui;

namespace vl {
namespace {
float noise(int n) {
    const uint32_t x=uint32_t(n)*747796405u+2891336453u;
    return float(((x^(x>>15))*277803737u)>>8)/16777216.f;
}

void dust(Renderer& r,float time,Color tint,bool ash) {
    for (int i=0;i<48;++i) {
        const float layer=noise(i+91),size=.7f+layer*2.2f;
        const float x=std::fmod(noise(i+3)*1320+time*(9+layer*21),1320.f)-60;
        const float y=std::fmod(noise(i+52)*420+time*(4+layer*14),420.f);
        const Vec p{x,ash?420-y:y};
        const float opacity=(.12f+layer*.36f)*(.68f+.32f*std::sin(time*1.5f+i));
        r.ellipse(p,{size,size*.62f},tint.alpha(opacity));
        if (layer>.85f) r.halo(p,7+size*4,tint.alpha(opacity*.45f));
    }
}

void radar(Renderer& r,float time,Color tint) {
    constexpr Vec center{996,196};
    for (int ring=1;ring<=3;++ring) {
        const float radius=ring*27.f;
        for (int s=0;s<32;++s) {
            const auto a=ray(s*pi/16),b=ray((s+1)*pi/16);
            r.line(center+a*radius,center+b*radius,1,tint.alpha(.30f));
        }
    }
    const float a=time*1.37f;
    for (int i=0;i<8;++i) {
        const float start=a-i*.064f;
        r.triangle(center,center+ray(start)*81,center+ray(start-.064f)*81,tint.alpha(.10f-i*.010f));
    }
    r.line(center,center+ray(a)*81,1.5f,tint.alpha(.85f));
    for (int i=0;i<3;++i) {
        const Vec p=center+ray(1.2f+i*2.05f)*(25+i*19.f);
        const float pulse=.45f+.28f*std::sin(time*3.5f+i);
        r.halo(p,18,tint.alpha(pulse));
        r.ellipse(p,{3,3},tint.alpha(.9f));
    }
}

void cutter(Renderer& r,float time,Color tint) {
    const Vec p=mix(Vec{1150,93},Vec{700,284},smooth(time/4.8f));
    const float shimmer=.72f+.16f*std::sin(time*38.f);
    r.beam(p+Vec{22,-3},9,p+Vec{196,-61},57,tint.alpha(.56f*shimmer),tint.alpha(0));
    r.halo(p+Vec{24,-4},29,tint.alpha(.68f));
    r.polygon(p+Vec{-45,13},p+Vec{7,-13},p+Vec{34,-5},p+Vec{10,10},hex(0x111b25));
    r.line(p+Vec{-30,7},p+Vec{14,-10},2,hex(0x95aebc).alpha(.7f));
    r.rect({p.x-14,p.y-2,10,3},hex(0xffdec0).alpha(shimmer));
}

void signal(Renderer& r,float time,Color tint,bool broken) {
    constexpr float x=867,y=112,w=244,h=63;
    r.rect({x,y,w,h},hex(0x08131c).alpha(.78f));
    r.outline({x,y,w,h},tint.alpha(.52f));
    r.line({x+9,y+h*.5f},{x+w-9,y+h*.5f},1,tint.alpha(.24f));
    Vec last{x+9,y+h*.5f};
    for (int i=1;i<65;++i) {
        const float a=i/64.f;
        const float wave=std::sin(a*59.f-time*14.f)*std::sin(a*19.f+time*3.f);
        const float gap=broken && std::fmod(a*6+time*.5f,2.f)>1.2f?.14f:1.f;
        const Vec next{x+9+a*(w-18),y+h*.5f+wave*19*gap};
        r.line(last,next,1.3f,tint.alpha(.82f));
        last=next;
    }
}

void landing(Renderer& r,float time,Color tint) {
    const float breathe=.75f+.24f*std::sin(time*6.f);
    for (int i=0;i<2;++i) {
        const Vec p{925.f+i*70,201.f-i*24};
        r.beam(p,8,p+Vec{-55.f+i*100,188},112,tint.alpha(.42f*breathe),tint.alpha(0));
        r.halo(p,44,tint.alpha(.60f*breathe));
        r.ellipse(p,{3,2},hex(0xffe1b7));
    }
}
}

void App::coopCinematic() {
    if (mission!=1 || !loadingMusic) {
        const auto& stage=*game.stage;
        r.cover(stage.art,screen,1.04f,{.52f,.48f},hex(0x91aab9));
        r.gradient(screen,hex(0x061018).alpha(.17f),hex(0x060b12).alpha(.94f));
        label({70,427},"CHAPTER "+digits(game.index+1)+"  /  CO-OP",teal);
        r.text({68,482},stage.name,53,white,true);
        const auto status=preloadStep<(mission==1?story::opening.size()+4:4)
            ?"Loading the mission":scheduleConfirmed?"Deployment confirmed":deployAt?"Confirming deployment":"Waiting for both operatives";
        r.text({72,519},status,18,pale);
        button({1008,516,136,42},"LEAVE",[this]{leaveCoop();});
        return;
    }

    size_t index=0;
    while (index+1<story::opening.size() && film>=story::opening[index+1].at) ++index;
    const auto& shot=story::opening[index];
    const float end=index+1<story::opening.size()?story::opening[index+1].at:net::openingDuration/1000.f;
    const float elapsed=std::max(0.f,film-shot.at);
    const float progress=smooth(elapsed/(end-shot.at));
    const float motion=settings.motion?film:0.f;
    const float reveal=smooth(elapsed/.66f);
    const Color tint=hex(shot.tint);

    if (index && reveal<1) {
        const auto& previous=story::opening[index-1];
        r.cover(previous.art,screen,settings.motion?1.15f:1.f,previous.end,
                mix(hex(0xcbd6dc),hex(previous.tint),.26f));
    } else r.rect(screen,hex(0x070c13));
    r.cover(shot.art,screen,settings.motion?mix(1.04f,1.15f,progress):1.f,
            settings.motion?mix(shot.start,shot.end,progress):Vec{.5f,.5f},
            mix(hex(0xcbd6dc),tint,.26f).alpha(index?reveal:std::max(.25f,reveal)));
    r.gradient(screen,hex(0x07111b).alpha(.23f),hex(0x080e17).alpha(.79f));
    r.gradient({0,335,width,246},hex(0x071019).alpha(0),hex(0x070e16).alpha(.97f));
    dust(r,motion,tint,index>=3);
    switch (index) {
    case 0: radar(r,motion,tint); break;
    case 1: signal(r,motion,tint,true); break;
    case 2: signal(r,motion,tint,false); break;
    case 3: cutter(r,settings.motion?elapsed:3.f,tint); break;
    case 4: radar(r,motion,tint); break;
    case 5: landing(r,motion,tint); break;
    }
    if (index==1 && settings.motion) {
        const float flicker=.06f+.06f*std::sin(motion*6.f);
        r.rect({0,32,width,300},hex(0x6cb6cf).alpha(flicker));
    }
    r.rect({0,0,width,32},hex(0x05090e));
    r.rect({0,570,width,30},hex(0x05090e));
    const float textAlpha=smooth((elapsed+.04f)/.42f);
    r.rect({63,363,45,3},tint.alpha(textAlpha));
    label({64,390},shot.location,tint.alpha(textAlpha));
    r.text({61,448},shot.title,index==5?50.f:48.f,white.alpha(textAlpha),true);
    label({66,480},shot.speaker,tint.alpha(textAlpha));
    copy({66,506},shot.line,890,20,pale.alpha(textAlpha));
    const float duration=net::openingDuration/1000.f;
    r.rect({66,579,1068,2},hex(0x243440));
    r.rect({66,579,1068*clamp(film/duration),2},tint);
    if (film>=duration) label({916,546},"WAITING FOR DEPLOYMENT",teal);
}
}
