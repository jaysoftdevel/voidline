#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"
#include "render/cinematic_gait.hpp"

using namespace vl::ui;

namespace vl {
namespace {
using story::Set;
constexpr Color night{.018f,.028f,.045f,1};

float noise(int index) {
    const auto n=uint32_t(index)*747796405u+2891336453u;
    return float(((n^(n>>15))*277803737u)>>8)/16777216.f;
}

void atmospheres(Renderer& r,float time,Color tint,bool ember) {
    for (int i=0;i<10;++i) {
        const float z=noise(i+18);
        const Vec at{std::fmod(noise(i+70)*1560+time*(9+z*15),1560.f)-180,250+z*197};
        r.puff(at,89+z*121,i*.73f+time*.035f,tint.alpha(.07f+z*.097f));
    }
    for (int i=0;i<45;++i) {
        const float z=noise(i+576),y=std::fmod(noise(i+308)*431+time*(4+z*25),431.f);
        const Vec at{std::fmod(noise(i+91)*1250+time*(5+z*16),1250.f)-25,ember?466-y:y+25};
        r.ellipse(at,{.65f+z*2,.45f+z*.83f},tint.alpha((.17f+z*.42f)*(.73f+.27f*std::sin(i+time*2.f))));
        if (z>.90f) r.halo(at,10,tint.alpha(.24f));
    }
}

void floor(Renderer& r,float horizon,float travel,Color tint,float wet) {
    const Color edge=mix(hex(0x354356),tint,.25f);
    r.imagePlane("cinematic/deck",{-217-travel,height},{width+220-travel,height},
        {width+78-travel*.25f,horizon-22},{-72-travel*.25f,horizon},hex(0x7e8e9e));
    r.gradient({0,horizon,width,height-horizon},night.alpha(.08f),night.alpha(.61f));
    r.line({0,horizon},{width,horizon-22},2,edge.alpha(.68f));
    r.line({0,horizon+2},{width,horizon-20},1,night.alpha(.84f));
    r.puff({711-travel*.7f,horizon+66},178,0,tint.alpha(.08f+wet*.19f));
    for (int i=0;i<9;++i) {
        const float x=214+i*109.f-travel;
        const float y=horizon+39+noise(i+33)*64;
        r.line({x,y},{x+14+noise(i+77)*40,y+1},1.1f,edge.alpha(.18f+wet*.22f));
    }
}

void frameSteel(Renderer& r,float shift,Color tint) {
    const float x=50+shift;
    r.polygon({x-104,0},{x+32,0},{x+2,457},{x-126,515},hex(0x0a121c));
    r.line({x+31,-10},{x+2,447},3,tint.alpha(.35f));
    r.line({x+21,38},{x-3,405},1,hex(0x366071).alpha(.68f));
    r.polygon({width-14+shift,0},{width+135+shift,0},{width+118+shift,543},{width-56+shift,518},hex(0x0a1118));
    r.line({width-13+shift,0},{width-55+shift,519},2,tint.alpha(.22f));
    for (int i=0;i<5;++i) {
        const float dx=i*38.f;
        r.line({x-70+dx,63},{x-42+dx,63},2,tint.alpha(.27f));
    }
}

void console(Renderer& r,Vec p,float size,float progress,float time,Color tint,bool physical) {
    const auto at=[&](float x,float y) { return p+Vec{x*size,y*size}; };
    const auto box=[&](float x,float y,float w,float h) { return Rect{p.x+x*size,p.y+y*size,w*size,h*size}; };
    const float pulse=.87f+.13f*std::sin(time*4);
    if (physical) {
        r.polygon(at(-56,-36),at(50,-36),at(40,21),at(-42,24),hex(0x14212e));
        r.gradient(box(-31,20,60,46),hex(0x263646),hex(0x121b26));
        r.line(at(-45,20),at(41,21),3*size,tint.alpha(.55f));
        r.halo(at(-7,-27),79*size,tint.alpha(.35f*pulse));
    }
    r.gradient(box(-55,-37,106,52),hex(0x122a3b).alpha(.95f),hex(0x081521).alpha(.97f));
    r.outline(box(-55,-37,106,52),tint.alpha(.56f),1.4f*size);
    for (int row=0;row<5;++row) {
        const float y=-29.f+8.f*row;
        const float length=17+noise(row*33+6)*24;
        r.line(at(-46,y),at(-46+length,y),1.25f*size,tint.alpha(.34f+pulse*.20f));
    }
    Vec prev=at(1,-12);
    for (int i=1;i<23;++i) {
        const float s=i/22.f;
        const float signal=std::sin(s*30-time*4.5f)*std::sin(s*pi)*9*progress;
        const Vec next=at(1+42*s,-12+signal);
        r.line(prev,next,1.3f*size,tint.alpha(.94f));
        prev=next;
    }
    r.line(at(0,3),at(43,3),2*size,hex(0x273949));
    r.line(at(0,3),at(43*progress,3),2*size,tint.alpha(.88f));
}

void farGate(Renderer& r,Vec p,float opening,Color tint,bool evac) {
    r.polygon(p+Vec{-131,-12},p+Vec{130,-19},p+Vec{126,86},p+Vec{-140,94},hex(0x101923));
    r.polygon(p+Vec{-112,-112},p+Vec{117,-127},p+Vec{125,-4},p+Vec{-119,1},hex(0x283d4d));
    r.polygon(p+Vec{-96,-101},p+Vec{99,-113},p+Vec{104,-5},p+Vec{-100,1},hex(0x060e17));
    const Color lamp=evac?hex(0xffc08a):tint;
    const Color metal=mix(hex(0x657485),tint,.16f);
    const float separation=97*opening;
    r.imageCrop("cinematic/deck",{p.x-113,p.y-113,229,15},{.05f,.03f,.50f,.14f},metal);
    r.gradient({p.x-96,p.y-98,192,96},lamp.energy(.16f),lamp.energy(.45f));
    r.halo(p+Vec{0,1},77,lamp.alpha(opening*.22f));
    r.polygon(p+Vec{-96,-4},p+Vec{96,-4},p+Vec{87,75},p+Vec{-111,82},hex(0x273543));
    r.polygon(p+Vec{-80,5},p+Vec{79,5},p+Vec{91,77},p+Vec{-102,83},lamp.energy(.28f));
    for (int i=1;i<5;++i) {
        const float y=5+i*i*3.f;
        r.line(p+Vec{-81-y*.25f,y},p+Vec{77+y*.16f,y-4},1.6f,lamp.alpha(.34f));
    }
    for (float dir:{-1.f,1.f}) {
        const Vec hinge=p+Vec{dir*98,-5},edge=p+Vec{dir*separation,-5},frame=hinge+Vec{dir*6,0};
        r.polygon(hinge-Vec{0,98},edge-Vec{0,98},edge,hinge,hex(0x344955));
        const float slab=std::abs(hinge.x-edge.x);
        if (slab>2.f) {
            r.imageCrop("cinematic/deck",{std::min(hinge.x,edge.x)+1,hinge.y-97,slab-2,93},
                        {.09f,.10f,slab/310.f,.36f},metal);
            r.line({std::min(hinge.x,edge.x)+1,hinge.y-96},{std::max(hinge.x,edge.x)-1,hinge.y-96},1.4f,hex(0x939fa8).alpha(.72f));
        }
        r.line(edge-Vec{0,98},edge,2,hex(0x778b8e).alpha(.8f));
        r.line(frame-Vec{0,99},frame+Vec{0,9},4,lamp.alpha(.55f+opening*.29f));
        r.halo(frame-Vec{0,44},47,lamp.alpha(.15f+opening*.18f));
        for (int j=0;j<4;++j) {
            const Vec a=mix(hinge,edge,.22f)-Vec{0,11.f+j*13.f};
            const Vec b=mix(hinge,edge,.70f)-Vec{0,11.f+j*13.f};
            r.line(a,b,1,hex(0x26353f).alpha(.89f));
        }
    }
    r.softShaft(p+Vec{0,-36},p+Vec{-103,86},125,lamp.alpha(opening*.32f));
    r.puff(p+Vec{-11,52},67,0,lamp.alpha(opening*.16f));
}

void capsule(Renderer& r,Vec p,float scale,Color tint,float opening) {
    const auto at=[&](float x,float y) { return p+Vec{x*scale,y*scale}; };
    r.polygon(at(-52,27),at(-40,-16),at(40,-20),at(62,21),hex(0x2a3a46));
    r.polygon(at(-52,27),at(62,21),at(59,43),at(-49,48),hex(0x111924));
    r.outline({p.x-27*scale,p.y-8*scale,58*scale,20*scale},tint.alpha(.71f),scale);
    r.line(at(-35,-13),at(-35,23),2*scale,hex(0x597683));
    r.line(at(37,-14),at(48,21),2*scale,hex(0x597683));
    const float slide=opening*29;
    r.imageCrop("cinematic/deck",{p.x-50*scale,p.y+(5-slide)*scale,110*scale,22*scale},{.34f,.51f,.32f,.44f},
                hex(0x7b8d9c).alpha(.94f));
    r.halo(at(13,8),49*scale,tint.alpha(.28f*opening));
    r.image("cinematic/recorder",{p.x-14*scale,p.y+2*scale,43*scale,14*scale},white.alpha(smooth(opening*2.2f)));
    r.halo(at(22,-2),31*scale,tint.alpha(.42f));
    r.ellipse(at(22,-2),{2.8f*scale,2.2f*scale},tint);
}

void boarding(Renderer& r,Vec center,float progress,Color tint) {
    const Vec start=center+Vec{-178,25};
    const Vec end=center+Vec{0,-33};
    r.line(start,end,13,hex(0x111b25));
    r.line(start-Vec{0,8},end-Vec{0,8},2,hex(0x768f9e));
    for (int i=0;i<8;++i) {
        const float u=(i+.5f)/8;
        const Vec a=mix(start,end,u);
        r.line(a,a-Vec{0,12},1.5f,tint.alpha(.48f));
    }
    for (int i=0;i<5;++i) {
        const float u=clamp(progress*1.45f-i*.153f);
        if (u>=1.f) continue;
        const Vec feet=mix(start,end,u)+Vec{-float(i)*12.f*(1-smooth(u/.29f)),0};
        const float k=mix(.95f,.54f,u);
        const float step=std::sin(u*25+i);
        const Color coat=mix(hex(0x263642),hex(0x71675a),noise(i+46)*.62f);
        r.ellipse(feet,{8*k,2.5f*k},night.alpha(.58f));
        const Vec hip=feet-Vec{0,18*k},neck=hip-Vec{0,14*k};
        r.line(hip,feet+Vec{step*5*k,0},4*k,coat.energy(.57f));
        r.line(hip,feet-Vec{step*5*k,2*k},4*k,coat.energy(.74f));
        r.line(hip,neck,11*k,coat);
        r.line(neck+Vec{2*k,1},hip+Vec{7*k,4},4*k,coat.energy(1.15f));
        r.ellipse(neck-Vec{0,6*k},{4.5f*k,5.2f*k},mix(coat,hex(0xb9a48e),.42f));
        r.line(neck-Vec{2*k,4*k},neck+Vec{4*k,-3*k},1.3f*k,tint.alpha(.70f));
    }
}

void cutterShot(Renderer& r,Vec center,float progress,Color tint,bool lifting) {
    const float flight=smooth(progress);
    const Vec nose=center+(lifting?Vec{227,-130}*flight:Vec{88,-107}*(1-flight));
    const float span=mix(250.f,303.f,lifting?1-flight:flight),rise=span/3;
    const Vec engine=nose-Vec{span*.87f,-rise*.14f};
    const float plume=lifting?.82f:.56f;
    r.softShaft(engine,engine+Vec{-160,58},112,tint.alpha(plume));
    r.halo(engine,48,tint.alpha(plume*.43f));
    r.sprite("cinematic/aster",nose,{span,-span*(lifting?.10f:.04f)},{rise*.08f,rise},{.92f,.55f},
        mix(hex(0xafbbca),tint,.19f));
    const Vec landing=nose+Vec{-29,23};
    r.softShaft(landing,landing+Vec{-76,193},213,hex(0xffddba).alpha(.70f));
    r.halo(landing,29,hex(0xffd4a5).alpha(.62f));
}

void outcome(Renderer& r,story::Action action,Vec at,float progress,float time,Color tint,bool wide,bool departure=false) {
    using story::Action;
    switch (action) {
    case Action::drain: {
        farGate(r,at,progress,tint,false);
        const float y=at.y-77+63*progress;
        r.gradient({at.x-62,y,124,at.y-y-5},tint.energy(.40f).alpha(.63f),night.alpha(.86f));
        for (int i=0;i<9;++i) {
            const float u=(i+.5f)/9, x=at.x-60+u*119;
            const float w=2.2f*std::sin(time*5+u*23);
            r.line({x,y+w},{x+12,y+w+1},1.1f,tint.alpha(.66f));
        }
        break;
    }
    case Action::relay: {
        console(r,at-Vec{0,71},wide?1.59f:2.20f,progress,time,tint,false);
        const Vec start=at+Vec{83,-63},end=at+Vec{193,-130};
        r.softShaft(start,end,32,tint.alpha(.68f*progress));
        const float phase=std::fmod(time*.60f,1.f);
        r.halo(mix(start,end,phase),18,tint.alpha(std::sin(pi*phase)*progress*.70f));
        break;
    }
    case Action::recover:
        capsule(r,at-Vec{0,62},wide?1.48f:2.35f,tint,progress);
        break;
    case Action::unlock:
        farGate(r,at,progress,tint,false);
        break;
    case Action::evacuate:
        farGate(r,at,smooth(.3f+progress),tint,true);
        boarding(r,at,progress,tint);
        break;
    case Action::launch:
        cutterShot(r,at-Vec{0,64},progress,tint,departure);
        break;
    }
}

struct SceneCast {
    const Character& lead;
    const Character* partner;
};

void renderSectorShot(Renderer& r,const Game& game,const story::Chapter& chapter,const SceneCast& cast,
                      int index,float elapsed,float duration,bool motion,int layer) {
    using story::Angle;
    const auto angle=story::cuts[index].angle;
    const bool arrival=angle==Angle::arrival,close=angle==Angle::close,detail=angle==Angle::detail;
    const bool aftermath=angle==Angle::aftermath;
    const bool reaction=angle==Angle::reaction;
    const auto action=story::actions[game.index];
    const bool launch=action.action==story::Action::launch;
    const bool finalLaunch=launch && game.index==sectorCount-1;
    const float t=motion?elapsed:duration*.52f, progress=cinematicEase(t/duration);
    const float phase=story::cuts[index].at+t;
    const bool pair=cast.partner!=nullptr;
    const bool forge=game.stage->biome==Biome::forge;
    const auto trim=hex(game.stage->accent),cool=hex(game.stage->cold);
    const auto glow=mix(cool,trim,.45f).energy(1.21f);
    const auto lamp=forge?hex(0xffad78):mix(glow,hex(0xe6f8ff),.34f);
    const float reveal=smooth((phase-8.15f)/14.30f)*action.strength;
    const float drift=mix(-26.f,24.f,progress);
    const std::string_view art=arrival?game.stage->art:story::scenery(chapter.set);
    const float zoom=arrival?mix(1.055f,1.12f,progress):close?mix(1.16f,1.24f,progress):
        detail?mix(1.17f,1.26f,progress):aftermath?mix(1.16f,1.07f,progress):mix(1.12f,1.19f,progress);
    const float biasX=arrival?mix(.35f,.59f,progress):detail?mix(.73f,.62f,progress):mix(.59f,.70f,progress);
    const float horizon=close?424.f:detail?405.f:arrival?383.f:398.f;
    r.beginCinematic(layer);
    r.cover(art,screen,zoom,{biasX,close?.54f:.43f},mix(white,cool,.08f));
    r.gradient(screen,night.alpha(.12f),night.alpha(.51f));
    r.softShaft({823+drift*.48f,-46},{579+drift,494},405,lamp.alpha(.67f));
    r.halo({816+drift*.42f,132},207,glow.alpha(.20f));
    atmospheres(r,phase,glow.energy(.81f),forge);
    floor(r,horizon,drift,glow,action.action==story::Action::drain?1-reveal:0.f);
    r.softShaft({1062+drift,-28},{613,475},310,glow.alpha(.18f));
    frameSteel(r,drift*.73f,glow);
    if (detail) {
        const float revealDetail=action.action==story::Action::recover?mix(.38f,reveal,.84f):reveal;
        outcome(r,action.action,{728-drift*.61f,350},revealDetail,phase,glow,false);
    } else if (!arrival) {
        const float actionProgress=finalLaunch?(aftermath?progress:1.f):reveal;
        const float outcomeX=launch?874.f:995.f;
        outcome(r,action.action,{outcomeX-drift*.48f,365},actionProgress,phase,glow,aftermath,finalLaunch && aftermath);
    }

    const bool walkOut=aftermath && (action.action==story::Action::evacuate || action.action==story::Action::recover);
    // An integrated acceleration curve keeps heel placement and displacement in lockstep.
    const float travel=arrival?233*cinematicCruise(t,0.f,3.95f,.72f):
        walkOut?230*cinematicCruise(t,.45f,4.83f,.78f):0.f;
    const float walk=motion?(arrival?cinematicWalk(t,0.f,3.95f,.72f):
        walkOut?cinematicWalk(t,.45f,4.83f,.78f):0.f):0.f;
    const float scale=arrival?2.82f:close?5.16f:detail?3.01f:reaction?4.26f:2.63f;
    const float x=(arrival?267.f:close?555.f:detail?392.f:reaction?(pair?458.f:572.f):453.f)-drift+travel;
    // Keep feet outside the subtitle-safe area, including the extreme close-up.
    const float y=close?465.f:detail?449.f:reaction?459.f:430.f;
    const float touch=close?smooth((phase-6.0f)/1.35f)*(1-smooth((phase-10.47f)/1.1f)):
        detail?.75f*smooth((phase-9.95f)/1.35f):0.f;
    const bool salvage=action.action==story::Action::recover;
    const Color fill=mix(hex(0xdfe5f3).energy(1.08f),mix(white,lamp,.26f).energy(1.22f),
                         close||detail?.72f:.51f);
    const auto light=mix(fill,glow.energy(1.44f),.19f*reveal);
    const float voiceTime=arrival?t:close||detail?phase-story::cuts[1].at:phase-story::cuts[3].at;
    // A gentle multi-frequency jaw envelope avoids rapidly flickering mouth poses.
    const float voice=.49f+.24f*std::sin(phase*6.7f)+.16f*std::sin(phase*11.4f+1.1f);
    const float speech=motion*cinematicEase((voiceTime-.38f)/.45f)*cinematicEase((5.5f-voiceTime)/.60f)
        *clamp(voice);
    const float stepIn=detail||close?.30f*touch:0.f;
    const Vec feet{x,y};
    // Props with physical mass belong to the rear plane. In v2.9.0 these were
    // emitted AFTER the operative, covering wrists, fingers and face in close shots.
    if (close) {
        const Vec panel{feet.x+158,feet.y-169+stepIn*45};
        if (salvage) capsule(r,panel+Vec{34,15},1.44f,glow,.3f*reveal);
        else console(r,panel,1.38f,cinematicEase((phase-7.42f)/3.70f),phase,glow,true);
    }
    r.ellipse(feet+Vec{2,1},{scale*13,scale*2.2f},night.alpha(.51f));
    if (pair) {
        const Vec others=feet+Vec{arrival?-144.f:close?-189.f:detail?-131.f:reaction?192.f:-110.f,-16};
        const float mateScale=scale*(reaction?.98f:.87f);
        const float respond=reaction?smooth((t-.47f)/.60f)*(1-smooth((t-2.14f)/.73f)):0.f;
        r.cinematicOperative(*cast.partner,{.feet=others,.scale=mateScale,.time=phase+1.43f,
            .walk=walk,.distance=travel+16,.reach=respond*.72f,.look=reaction?-.27f:.05f,
            .speech=reaction?speech:0,.turn=mix(.88f,.69f,respond),.facing=reaction?-1:1,
            .handOffset={13,-.8f},.light=mix(light,glow,.12f).energy(.89f),.rim=trim,.carried={}});
    }
    const float look=close||detail?mix(.18f,.43f,touch):reaction?mix(-.20f,.06f,progress):-.19f;
    r.cinematicOperative(cast.lead,{.feet=feet,.scale=scale,.time=phase,.walk=walk,.distance=travel,
        .reach=walkOut && salvage?1.f:touch,.crouch=salvage?stepIn:stepIn*.37f,.look=look,
        .speech=arrival||(!pair && reaction)?speech:0,.turn=reaction?.56f:.83f,
        .handOffset=walkOut && salvage?Vec{10,14}:Vec{mix(15.f,20.0f,touch),mix(5.f,7.7f,touch)},
        .light=light,.rim=lamp,.carried=(walkOut && salvage)?"cinematic/recorder":""});
    // Subtle atmospheric glows are safe in front; steel frame and full-width
    // shafts are behind the actor so a silhouette is never cut by a prop.
    r.puff({781+t*21,452},171,phase*.051f,glow.alpha(.035f));
}
}

void App::sectorCinematic() {
    const auto& chapter=story::chapters[game.index];
    const auto duration=story::duration(game.index);
    const int shot=story::shotAt(film);
    const auto& cut=story::cuts[shot];
    const float elapsed=film-cut.at;
    const auto shotLength=[&](int n) {
        return (n+1==static_cast<int>(story::cuts.size())?duration:story::cuts[n+1].at)-story::cuts[n].at;
    };
    constexpr float dissolveTime=.54f;
    const bool blend=shot>0 && elapsed<dissolveTime;
    const bool pair=game.coop;
    const bool swap=pair && !network.hosting();
    const SceneCast cast{swap?*game.teammate().character:*game.character,
        pair?(swap?game.character:game.teammate().character):nullptr};
    // Hold outgoing shot on its final pose throughout the dissolve. Otherwise
    // the previous camera/characters creep forward during a cut.
    if (blend) renderSectorShot(r,game,chapter,cast,shot-1,shotLength(shot-1),shotLength(shot-1),settings.motion,0);
    renderSectorShot(r,game,chapter,cast,shot,elapsed,shotLength(shot),settings.motion,blend?1:0);
    const Color accent=hex(game.stage->accent),cold=hex(game.stage->cold);
    const float dissolve=blend?smooth(elapsed/dissolveTime):1.f;
    const float fade=std::min(smooth(film/.61f),smooth((duration-film)/.74f));
    r.finishCinematic(settings.motion?film:0.f,fade,mix(cold,accent,.40f),dissolve,{.53f,.50f},settings.high?.33f:.15f);

    r.rect({0,0,width,28},night);
    // Subtitle matte starts below the operative's feet instead of fogging the legs.
    r.gradient({0,481,width,67},night.alpha(.08f),night.alpha(.96f));
    r.rect({0,548,width,52},night);
    const float titleAlpha=smooth(film/.49f);
    r.text({59,74},game.stage->name,15,pale.alpha(titleAlpha),true);
    r.outlinedText({56,112},chapter.title,34,white.alpha(titleAlpha));
    if (cut.angle==story::Angle::aftermath) {
        const float revealAlpha=smooth((elapsed-.38f)/.70f);
        const float titleWidth=r.measure(chapter.reveal,13,true);
        r.rect({1122-titleWidth-12,51,titleWidth+29,25},night.alpha(.88f*revealAlpha));
        r.rect({1122-titleWidth-12,51,2,25},accent.alpha(revealAlpha));
        r.text({1122-titleWidth-3,69},chapter.reveal,13,teal.alpha(revealAlpha),true);
    }
    const bool carryCaption=blend && story::cuts[shot-1].dialogue!=cut.dialogue && dissolve<.49f;
    const int dialogue=carryCaption?story::cuts[shot-1].dialogue:cut.dialogue;
    const auto& line=chapter.lines[dialogue];
    const Character& second=pair?*cast.partner:cast.lead;
    const std::string_view speaker=line.voice==story::Voice::lead?cast.lead.name:
        line.voice==story::Voice::partner?second.name:
        line.voice==story::Voice::archive?"HELIX SYSTEM LOG":line.voice==story::Voice::aster?"RESCUE CUTTER ASTER":"NERI / CIVILIAN COMMS";
    const auto speakTint=line.voice==story::Voice::lead?hex(cast.lead.color):
        line.voice==story::Voice::partner?hex(second.color):teal;
    const float textAlpha=(blend && story::cuts[shot-1].dialogue!=cut.dialogue?std::abs(dissolve*2-1):1.f)*
        titleAlpha*smooth((duration-film)/.60f);
    r.text({59,482},speaker,13,speakTint.alpha(textAlpha),true);
    copy({59,510},line.text,863,22,white.alpha(textAlpha));
    r.rect({58,571,1084,2},hex(0x293b4b));
    r.rect({58,571,1084*clamp(film/duration),2},accent);
    if (!coopStarted || network.hosting())
        button({1010,493,132,34},"SKIP SCENE",[this]{skipEpilogue();});
}
}
