#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
float Ambient::pulse() const {
    const float t=1-life/maxLife;
    if (kind==AmbientKind::blast) return std::exp(-t*5.5f)*std::min(1.f,t/.02f)*std::min(1.f,size);
    if (kind==AmbientKind::lightning) {
        const float a=(t-.06f)*34, b=(t-.2f)*24, c=(t-.42f)*30;
        return std::exp(-a*a)+.75f*std::exp(-b*b)+.45f*std::exp(-c*c);
    }
    return 0;
}

void Game::spawnAmbient(int forcedKind,int forcedVariant) {
    struct Pick { AmbientKind kind; int variant; float weight; };
    static const std::vector<Pick> table[5]{
        {{AmbientKind::blast,0,3},{AmbientKind::flyer,0,2},{AmbientKind::streak,0,1},{AmbientKind::searchlight,0,2},{AmbientKind::fountain,0,1}},
        {{AmbientKind::steam,0,3},{AmbientKind::fountain,1,3},{AmbientKind::flyer,2,2},{AmbientKind::flyer,0,1}},
        {{AmbientKind::spores,0,4},{AmbientKind::flyer,1,3},{AmbientKind::streak,0,1},{AmbientKind::fountain,3,1}},
        {{AmbientKind::fountain,2,3},{AmbientKind::blast,1,2},{AmbientKind::steam,1,1},{AmbientKind::flyer,0,1}},
        {{AmbientKind::lightning,0,3},{AmbientKind::flyer,0,3},{AmbientKind::streak,0,2},{AmbientKind::searchlight,1,1}}
    };
    const auto& options=table[std::clamp(index,0,4)];
    float total=0;
    for (const auto& o:options) total+=o.weight;
    float roll=ambientRandom()*total;
    Pick chosen=options.front();
    for (const auto& o:options) { if ((roll-=o.weight)<=0) { chosen=o; break; } }
    if (forcedKind>=0) chosen={static_cast<AmbientKind>(forcedKind),forcedVariant,1};
    const Pick* pick=&chosen;
    auto R=[&]{ return ambientRandom(); };
    Ambient a{};
    a.kind=pick->kind; a.variant=pick->variant; a.seed=R()*997;
    const float dir=R()<.5f?-1.f:1.f;
    switch (a.kind) {
    case AmbientKind::blast:
        a.depth=.16f+R()*.2f; a.size=.55f+R()*.95f; a.life=a.maxLife=1.9f;
        a.p={camera*a.depth+130+R()*940,190+R()*200};
        if (a.variant==1) { a.p.y=330+R()*70; a.size=std::max(a.size,.9f); }
        break;
    case AmbientKind::flyer: {
        a.depth=a.variant==1?.3f:.12f; a.size=a.variant==0?(index==4?1.5f:.9f):.8f+R()*.4f;
        const float speed=a.variant==0?44+R()*30:a.variant==1?120+R()*60:38+R()*22;
        a.v={dir*speed,0};
        a.p={camera*a.depth+(dir>0?-240.f:1440.f),a.variant==2?545+R()*35:a.variant==1?80+R()*200:(index==4?60+R()*130:80+R()*120)};
        a.life=a.maxLife=1700/speed;
        break;
    }
    case AmbientKind::fountain:
        a.depth=.22f; a.size=.8f+R()*.5f; a.life=a.maxLife=2.6f;
        a.p={camera*a.depth+150+R()*900,a.variant==1?556.f:a.variant==2?410+R()*30:a.variant==3?520.f:330+R()*60};
        break;
    case AmbientKind::streak:
        a.depth=.04f; a.size=1; a.life=a.maxLife=.95f;
        a.p={camera*a.depth+150+R()*900,16+R()*70}; a.v={dir*(480+R()*220),240+R()*120};
        break;
    case AmbientKind::lightning:
        a.depth=.02f; a.size=1; a.life=a.maxLife=.8f;
        a.p={camera*a.depth+150+R()*900,330+R()*40};
        break;
    case AmbientKind::searchlight:
        a.depth=.1f; a.size=1; a.life=a.maxLife=7.f;
        a.p={camera*a.depth+(dir>0?40.f:1160.f),600.f};
        a.v={dir>0?-1.35f:-1.8f,dir*.34f};
        break;
    case AmbientKind::steam:
        a.depth=.2f; a.size=.8f+R()*.5f; a.life=a.maxLife=2.4f;
        a.p={camera*a.depth+130+R()*940,250+R()*170};
        break;
    case AmbientKind::spores:
        a.depth=.18f+R()*.1f; a.size=.8f+R()*.6f; a.life=a.maxLife=4.5f;
        a.p={camera*a.depth+130+R()*940,380+R()*160};
        break;
    }
    // Distant detonations rumble the ground a little and are heard muffled.
    if (a.kind==AmbientKind::blast) {
        shake=std::max(shake,1.6f*a.size);
        audio.event("destroy",.10f+.12f*a.size,(a.p.x-a.depth*camera-width*.5f)/width,.42f+R()*.1f);
    } else if (a.kind==AmbientKind::lightning) {
        audio.event("destroy",.16f,R()-.5f,.34f);
    } else if (a.kind==AmbientKind::fountain && a.variant==2) {
        audio.event("hit-metal",.08f,R()-.5f,.4f);
    }
    ambient.push_back(a);
}

void Game::updateAmbient(float dt) {
    ambientTimer-=dt;
    if (ambientTimer<=0 && ambient.size()<6) {
        spawnAmbient();
        ambientTimer=1.6f+ambientRandom()*3.6f;
    }
    float flashAmount=0;
    Color flashColor=hex(stage->cold);
    for (auto& a:ambient) {
        a.life-=dt;
        if (a.kind!=AmbientKind::searchlight) a.p+=a.v*dt;
        const float pulse=a.pulse();
        if (pulse>flashAmount) {
            flashAmount=pulse;
            flashColor=a.kind==AmbientKind::lightning?hex(0xdbe8ff):a.variant==1?hex(0xff9a4a):hex(stage->accent);
        }
    }
    skyFlash=flashAmount; skyColor=flashColor;
    std::erase_if(ambient,[](const Ambient& a){return a.life<=0;});
}
} // namespace vl
