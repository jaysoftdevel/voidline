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
    if (mission!=1 || !loadingMusic) {
        const auto& stage=*game.stage;
        r.cover(stage.art,screen,1.04f,{.52f,.48f},hex(0x91aab9));
        r.gradient(screen,hex(0x061018).alpha(.17f),hex(0x060b12).alpha(.94f));
        label({70,427},"CHAPTER "+digits(game.index+1)+"  /  CO-OP",teal);
        r.text({68,482},stage.name,53,white,true);
        const size_t assets=story::missionPreloads+(mission==1?story::opening.size():0);
        const auto status=preloadStep<assets
            ?"Loading the mission":scheduleConfirmed?"Deployment confirmed":deployAt?"Confirming deployment":"Waiting for both operatives";
        r.text({72,519},status,18,pale);
        button({1008,516,136,42},"LEAVE",[this]{leaveCoop();});
        return;
    }

    drawOpening(true);
}

void App::drawOpening(bool team) {
    int index=0;
    while (index+1<static_cast<int>(story::opening.size()) && film>=story::opening[index+1].at) ++index;
    const float duration=net::openingDuration/1000.f;
    const auto length=[&](int n) {
        return (n+1==static_cast<int>(story::opening.size())?duration:story::opening[n+1].at)-story::opening[n].at;
    };
    const auto& shot=story::opening[index];
    const float elapsed=std::max(0.f,film-shot.at);
    const float reveal=smooth(elapsed/.55f);
    const bool blend=index>0 && elapsed<.55f;
    const auto render=[&](int n,float time,int layer) {
        const auto& take=story::opening[n];
        const float shotTime=settings.motion?time:length(n)*.55f;
        const float p=cinematicEase(shotTime/length(n));
        const Color tint=hex(take.tint);
        r.beginCinematic(layer);
        r.cover(take.art,screen,mix(1.05f,1.15f,p),mix(take.start,take.end,p),
                mix(hex(0xdce5ee),tint,.18f));
        r.gradient(screen,hex(0x07111b).alpha(.21f),hex(0x080e17).alpha(.78f));
        dust(r,shotTime+take.at,tint,n>=3);
        switch (n) {
        case 0: radar(r,shotTime,tint); break;
        case 1: signal(r,shotTime,tint,true); break;
        case 2: signal(r,shotTime,tint,false); break;
        case 3: cutter(r,shotTime,tint); break;
        case 4: radar(r,shotTime,tint); break;
        case 5: {
            landing(r,shotTime,tint);
            const bool swap=team && !network.hosting();
            const auto& lead=team?(swap?*game.teammate().character:*game.character):characters[save.character];
            const float distance=231*cinematicCruise(shotTime,0.f,4.32f,.74f);
            const float gait=settings.motion?cinematicWalk(shotTime,0.f,4.32f,.74f):0.f;
            r.imagePlane("cinematic/deck",{-130,height},{width+125,height},{width+59,395},{-61,407},hex(0x667988));
            r.softShaft({901,192},{573,496},335,tint.alpha(.71f));
            const Vec feet{403.f+distance-p*36,478};
            if (team) {
                const auto& mate=swap?*game.character:*game.teammate().character;
                r.cinematicOperative(mate,{.feet=feet-Vec{151,19},.scale=3.12f,.time=shotTime+1.05f,
                    .walk=gait,.distance=distance+29,.look=-.16f,.light=hex(0xb1c2d7).energy(1.19f),.rim=tint,.carried={}});
            }
            r.cinematicOperative(lead,{.feet=feet,.scale=3.88f,.time=shotTime,.walk=gait,.distance=distance,
                 .look=mix(-.06f,-.36f,p),.light=mix(white,tint,.16f).energy(1.12f),.rim=tint,.carried={}});
            break;
        }
        }
    };
    if (blend) render(index-1,length(index-1),0);
    render(index,elapsed,blend?1:0);
    const Color tint=hex(shot.tint);
    r.finishCinematic(settings.motion?film:0.f,smooth(film/.57f),tint,blend?reveal:1.f,
        {.66f,.46f},index==5?.22f:.05f);

    // The original full-width gradient covered the operative from the waist down.
    // Reserve the dark matte for the subtitle band below the actors' boots.
    r.gradient({0,493,width,76},hex(0x071019).alpha(.08f),hex(0x070e16).alpha(.94f));
    r.rect({0,0,width,27},hex(0x05090e));
    r.rect({0,569,width,31},hex(0x05090e));
    const auto& caption=blend && reveal<.5f?story::opening[index-1]:shot;
    const float textAlpha=index==0?smooth(elapsed/.45f):blend?std::abs(reveal*2-1):1.f;
    r.rect({63,392,37,2},tint.alpha(textAlpha));
    label({64,413},caption.location,tint.alpha(textAlpha));
    r.text({61,457},caption.title,41,white.alpha(textAlpha),true);
    label({65,485},caption.speaker,tint.alpha(textAlpha));
    copy({65,512},caption.line,817,20,pale.alpha(textAlpha));
    r.rect({65,580,1070,2},hex(0x243440));
    r.rect({65,580,1070*clamp(film/duration),2},tint);
}
}
