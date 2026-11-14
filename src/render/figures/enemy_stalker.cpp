#include "render/figures/enemy_view.hpp"

namespace vl {
// Quadruped hunter. Diagonal gait, an undulating spine, a lashing tail and a crouch before the lunge.
// When it dies its legs twitch, curl in toward the belly and stiffen.
void Renderer::drawStalker(const EnemyView& v) {
    const Enemy& e=v.e;
    const Color black=v.black,hull=v.hull,rim=v.rim,plate=v.plate,alert=v.alert,accent=v.accent;
    const float time=v.time;
    const float squash=e.landed*3.f;
    const float lunge=clamp(-e.vertical/450,0,1), fall=clamp(e.vertical/450,0,1);
    const float phase=e.distance*.07f;
    const float run=v.pace;
    const Vec ground=v.ground;
    const Vec body=v.center+Vec{0,squash+(e.vertical==0?std::cos(phase*2)*1.2f*run:0.f)};
    const float dirx=v.turn;
    const float pitch=(lunge*-.2f+fall*.25f)*dirx;
    auto bodyAt=[&](float lx,float ly) { return body+Vec{lx*dirx,ly+lx*pitch*dirx}; };
    const float curl=e.alive?0.f:smooth(v.dead/.9f);
    // Twitching dies away as the machine shuts down.
    const float twitch=e.alive?0.f:std::sin(v.time*34.f)*(1-curl)*4.f;
    struct Leg { float lx; int pair; };
    const Leg legs[4]{{-14.f,1},{-9.f,0},{9.f,0},{14.f,1}};
    for (int pass=0;pass<2;++pass) {
        for (int i=0;i<4;++i) {
            const bool nearSide=(i%2)==1;
            if ((pass==0)==nearSide) continue;
            const auto& leg=legs[i];
            const float cyc=phase+leg.pair*pi;
            const float swing=run>.05f?std::sin(cyc):0.f;
            const float rise=run>.05f?std::max(0.f,std::cos(cyc)):0.f;
            Vec foot=ground+Vec{leg.lx*dirx*1.5f*(1+.2f*(i<2?1:-1))+swing*10.f*run*dirx,-rise*7.f*run};
            if (e.vertical!=0) foot=bodyAt(leg.lx*1.3f+(i<2?-1.f:1.f)*lunge*10.f,12.f+lunge*8.f-fall*4.f);
            const Vec hipJoint=bodyAt(leg.lx,3.f);
            if (!e.alive) {
                // Legs fold up toward the belly.
                const Vec folded=bodyAt(leg.lx*.8f+(i<2?-3.f:3.f),9.f+(nearSide?-2.f:1.f));
                foot=mix(foot,folded+Vec{twitch*(nearSide?1.f:-1.f),twitch*.5f},curl);
            }
            const int knee=(i<2?-1:1)*(dirx>=0?1:-1);
            const Vec kneeP=solveKnee(hipJoint,foot,13.f,14.f,knee);
            const Color tint=nearSide?hull:hull.energy(.66f);
            limb(hipJoint,kneeP,nearSide?5.f:4.2f,tint,rim);
            limb(kneeP,foot,nearSide?4.2f:3.4f,tint.energy(.9f),rim.energy(.7f));
            line(foot-Vec{1.5f*dirx,0},foot+Vec{4.f*dirx,0},2.4f,black);
            line(foot+Vec{3.f*dirx,0},foot+Vec{5.5f*dirx,-1.2f},1.4f,rim);
            if (nearSide) ellipse(kneeP,{2.3f,2.3f},plate);
        }
    }
    // Tail with three trailing segments. It goes limp when the machine dies.
    Vec from=bodyAt(-17.f,-1.f);
    for (int k=1;k<=4;++k) {
        const float lash=std::sin(time*6.f-k*.9f+e.phase)*2.2f*(1+run)*(1-curl);
        const Vec to=bodyAt(-17.f-k*6.5f,-2.f-k*1.4f*(1-curl)+lash+curl*k*1.2f);
        limb(from,to,std::max(1.8f,5.2f-k*.8f),hull.energy(.8f),rim.energy(.6f));
        from=to;
    }
    triangle(from+Vec{-1.f*dirx,-2.f},from+Vec{-6.f*dirx,-4.f},from+Vec{-1.f*dirx,1.5f},accent.energy(1.8f*(1-v.burnt)));
    // Segmented chassis, dorsal vents and head pod.
    ellipse(body,{19.f,10.2f},black);
    ellipse(body+Vec{0,-1.f},{17.2f,8.f},hull);
    for (int k=-3;k<=3;++k) {
        const Vec seg=bodyAt(k*4.6f,-7.f);
        line(seg,seg+Vec{0,6.f},.9f,black.alpha(.55f));
        if (k%2==0) line(bodyAt(k*4.6f-1.4f,-8.f),bodyAt(k*4.6f+1.4f,-8.f),1.4f,accent.energy((1.3f+.4f*std::sin(time*5.f+k))*(1-v.burnt)));
    }
    line(bodyAt(-15.f,-5.5f),bodyAt(14.f,-6.f),1.f,rim.alpha(.7f));
    const Vec head=bodyAt(20.f,-1.f+v.breath*.6f+lunge*-2.f+curl*3.f);
    ellipse(head,{9.5f,7.2f},black);
    ellipse(head+Vec{-.5f*dirx,-.6f},{8.2f,6.f},hull.energy(1.05f));
    const float jaw=e.alive?std::sin(time*9.f)*.8f:curl*2.4f;
    for (int k=-1;k<=1;k+=2) {
        line(head+Vec{5.f*dirx,2.5f*k},head+Vec{12.f*dirx,5.f*k+jaw},1.8f,black);
        line(head+Vec{5.f*dirx,2.5f*k},head+Vec{11.f*dirx,4.6f*k+jaw},.9f,rim);
    }
    emissive(head+Vec{3.5f*dirx,-2.4f},2.1f,alert,e.hp<e.maxHp?2.6f:1.9f);
    emissive(head+Vec{6.f*dirx,-.8f},1.4f,alert,1.5f);
    // A damaged stalker leaks sparks from a cracked chassis.
    if (e.alive && e.hp<e.maxHp*.5f)
        line(bodyAt(-6.f,-8.f),bodyAt(-2.f,-3.f),1.f,hex(0xffc27a).energy(1.8f));
}
} // namespace vl
