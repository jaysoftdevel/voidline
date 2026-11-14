#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/coop_story.hpp"
#include "render/cinematic_gait.hpp"

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
    const float arrival=smooth(time/5.f);
    const Vec nose=mix(Vec{1150,139},Vec{865,302},arrival);
    const float span=mix(157.f,318.f,arrival);
    const Vec engine=nose-Vec{span*.87f,-span*.040f};
    r.softShaft(engine,engine+Vec{-128,42},93,tint.alpha(.70f));
    r.halo(engine,45,tint.alpha(.66f));
    r.sprite("cinematic/aster",nose,{span,-span*.055f},{span*.022f,span/3},{.92f,.55f},
             mix(hex(0xbcc9d9),tint,.19f));
    r.softShaft(nose+Vec{-19,13},nose+Vec{-69,151},162,tint.alpha(.54f));
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
    if (mission!=1 || !loadingMusic || openingSkipped) {
        const auto& stage=*game.stage;
        r.cover(stage.art,screen,1.04f,{.52f,.48f},hex(0x91aab9));
        r.gradient(screen,hex(0x061018).alpha(.17f),hex(0x060b12).alpha(.94f));
        label({70,427},"CHAPTER "+digits(game.index+1)+"  /  CO-OP",teal);
        r.text({68,482},stage.name,53,white,true);
        const size_t assets=story::missionPreloads+(mission==1?story::opening.size():0);
        const auto status=preloadStep<assets
            ?"Loading the mission":openingSkipped?"Prologue skipped  /  synchronizing deployment":scheduleConfirmed?"Deployment confirmed":deployAt?"Confirming deployment":"Waiting for both operatives";
        r.text({72,519},status,18,pale);
        button({1008,516,136,42},"LEAVE",[this]{leaveCoop();});
        return;
    }

    drawOpening(true);
    button({995,528,160,35},"SKIP PROLOGUE",[this]{skipPrologue();});
}

void App::drawOpening(bool team) {
    (void)team;
    int index=0;
    while (index+1<static_cast<int>(story::opening.size()) && film>=story::opening[index+1].at) ++index;
    const float duration=net::openingDuration/1000.f;
    const auto length=[&](int n) {
        return (n+1==static_cast<int>(story::opening.size())?duration:story::opening[n+1].at)-story::opening[n].at;
    };
    const auto& shot=story::opening[index];
    const auto render=[&](int n,int layer) {
        const auto& take=story::opening[n];
        const float shotTime=clamp(film-take.at,0.f,length(n));
        const float p=cinematicEase(shotTime/length(n));
        const Color tint=hex(take.tint);
        r.beginCinematic(layer);
        // Dolly/zoom is slow, continuous and tied to the shot's own time.
        // No character models are drawn into the prologue; environmental
        // artwork is the visual narrative, with procedural cinematic overlays.
        r.cover(take.art,screen,mix(1.09f,1.19f,p),mix(take.start,take.end,p),
                mix(hex(0xdce5ee),tint,.17f));
        r.gradient(screen,hex(0x07111b).alpha(.18f),hex(0x080e17).alpha(.73f));
        dust(r,shotTime+take.at,tint,n>=3);
        switch (n) {
        case 0: radar(r,shotTime,tint); break;
        case 1: signal(r,shotTime,tint,true); break;
        case 2:
            signal(r,shotTime,tint,true);
            for (int i=0;i<5;++i) {
                const float blink=.23f+.65f*(std::sin(shotTime*5+i*2.4f)>.4f);
                r.rect({927.f+i*23,187,13,5},hex(0xf5a08b).alpha(blink));
            }
            break;
        case 3: signal(r,shotTime,tint,false); break;
        case 4: cutter(r,shotTime,tint); break;
        case 5: radar(r,shotTime,tint); break;
        case 6:
            landing(r,shotTime,tint);
            r.imagePlane("cinematic/deck",{-130,height},{width+125,height},
                         {width+59,395},{-61,407},hex(0x667988));
            r.softShaft({901,192},{573,496},335,tint.alpha(.45f));
            break;
        }
        // Animated scan band and occasional emergency warning flash.
        const float scan=std::fmod(shotTime*92+float(n)*107,560.f);
        r.line({0,scan},{width,scan},1,tint.alpha(.055f));
        if (n==2 && std::sin(shotTime*7)>.94f)
            r.rect(screen,hex(0xea644d).alpha(.043f));
    };
    const float since=film-story::opening[index].at;
    const float transition=cinematicEase(since/.72f);
    if (index>0 && since<.72f) {
        render(index-1,0);
        render(index,1);
    } else render(index,0);
    const Color tint=hex(shot.tint);
    r.finishCinematic(film,1.f,tint,index>0 && since<.72f?transition:1.f,{.66f,.46f},index==6?.20f:.05f);

    // Story overlays live in a protected band below the image focal point.
    r.gradient({0,493,width,76},hex(0x071019).alpha(.08f),hex(0x070e16).alpha(.94f));
    r.rect({0,0,width,27},hex(0x05090e));
    r.rect({0,569,width,31},hex(0x05090e));
    const float captionOpacity=smooth(std::max(0.f,since-.38f)/.48f);
    r.rect({63,392,37*captionOpacity,2},tint);
    label({64,413},shot.location,tint.alpha(captionOpacity));
    r.text({61,457},shot.title,41,white.alpha(captionOpacity),true);
    label({65,485},shot.speaker,tint.alpha(captionOpacity));
    copy({65,512},shot.line,817,20,pale.alpha(captionOpacity));
    // Progress bar follows the actual time in every shot.
    for (int page=0;page<static_cast<int>(story::opening.size());++page) {
        constexpr float gap=5.f;
        const float unit=(1070.f-gap*(story::opening.size()-1))/story::opening.size();
        const float filled=page<index?1.f:page==index?clamp(since/length(index)):0.f;
        r.rect({65+page*(unit+gap),580,unit,2},hex(0x243440));
        if (filled>0) r.rect({65+page*(unit+gap),580,unit*filled,2},tint);
    }
}
}
