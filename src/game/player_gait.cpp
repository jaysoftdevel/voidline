#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::updateGait(float dt,float travel,float accel,int priorSupport,float landing) {
    auto& p=player;
    auto& g=p.gait;
    const float speed=std::abs(p.velocity.x), pace=clamp(speed/(character->speed*speedScale),0,1.28f);
    const bool onGround=p.support>=0;
    const float center=p.box.center().x, floor=p.box.bottom();
    g.crouch=mix(g.crouch,p.crouched?1.f:0.f,1-std::exp(-(p.crouched?29:24)*dt));
    g.run=mix(g.run,smooth((pace-.29f)/.88f)*(1-g.crouch),1-std::exp(-11*dt));
    const float tuckTarget=onGround?0.f:smooth((p.jumpAge-.055f)/.14f)*(1-smooth((p.velocity.y-45)/610));
    g.tuck=mix(g.tuck,tuckTarget,1-std::exp(-14*dt));
    g.reach=mix(g.reach,onGround?0.f:smooth(p.velocity.y/510),1-std::exp(-13*dt));
    const float anticip=p.windup>0?3.4f*(1-p.windup/windupDuration):0.f;
    const float bob=std::cos(g.cycle*4*pi-.4f)*g.weight*(.9f+g.run*1.2f);
    g.pelvis=mix(g.pelvis,bob+anticip,1-std::exp(-28*dt));
    g.weight=mix(g.weight,onGround?smooth(speed/65):0.f,1-std::exp(-14*dt));
    g.lean=mix(g.lean,clamp(p.velocity.x/(character->speed*speedScale)*(.15f+g.run*.11f)+accel*.000061f,-.39f,.39f),1-std::exp(-11*dt));
    if (landing>0) g.compressVelocity+=landing*124;
    g.compressVelocity+=(-161*g.compression-20*g.compressVelocity)*dt;
    g.compression=std::max(0.f,g.compression+g.compressVelocity*dt);
    if (g.compression==0 && g.compressVelocity<0) g.compressVelocity=0;

    const int oldDirection=g.direction;
    if (speed>25) g.direction=sign(p.velocity.x);
    const bool changedSurface=p.support!=priorSupport;
    if (onGround && changedSurface) {
        // Lead with the foot nearest the surface. Swing the other into the next step.
        const int lead=g.feet[0].heel.y>=g.feet[1].heel.y?0:1;
        g.cycle=lead==0?0.f:.5f;
    }
    if (onGround && oldDirection!=g.direction) {
        const int lead=g.feet[0].planted?0:1;
        g.cycle=lead==0?.18f:.68f;
    }
    const float stance=mix(mix(.62f,.39f,g.run),.69f,g.crouch);
    const float cycleLength=mix(mix(47.f,118.f,g.run),42.f,g.crouch);
    if (onGround) g.cycle=std::fmod(g.cycle+std::abs(travel)/cycleLength,1.f);

    for (int i=0;i<2;++i) {
        auto& foot=g.feet[i];
        const float phase=std::fmod(g.cycle+.5f*i,1.f);
        if (!onGround) {
            foot.surface=-1; foot.planted=false;
            const bool trail=i==int(g.launchFoot);
            const float ahead=trail?mix(-13.f,-6.f,g.reach):mix(16.f,11.f,g.reach);
            const float rise=(trail?6.f:13.f)*g.tuck+(trail?7.f:2.f)*(1-g.reach);
            const Vec target{center+g.direction*ahead,floor-rise};
            foot.heel=mix(foot.heel,target,1-std::exp(-dt*(13+15*g.reach)));
            const float extension=trail?-.47f:.26f;
            foot.angle=mix(foot.angle,extension*(1-g.reach)+.13f*g.reach,1-std::exp(-15*dt));
        } else {
            const auto& surface=platforms[p.support].box;
            const float rest=center+(i==0?-5.f:5.f);
            const float reach=cycleLength*stance*.46f*g.weight;
            const float front=clamp(center+reach*g.direction,surface.x+2,surface.right()-2);
            if (speed<13) {
                if (foot.surface!=p.support || !foot.planted || std::abs(foot.heel.x-rest)>12) {
                    foot.heel=mix(foot.heel,{clamp(rest,surface.x+2,surface.right()-2),floor},1-std::exp(-19*dt));
                    foot.anchor=foot.heel.x-surface.x;
                } else {
                    foot.heel={surface.x+foot.anchor,floor};
                }
                foot.surface=p.support; foot.planted=true;
                foot.angle=approach(foot.angle,0,dt*8);
            } else if (phase<stance) {
                const bool touchdown=!foot.planted || phase<foot.phase || foot.surface!=p.support;
                if (touchdown) {
                    const float projected=changedSurface?foot.heel.x:front;
                    foot.anchor=clamp(clamp(projected,center-24,center+24),surface.x+2,surface.right()-2)-surface.x;
                    if (!changedSurface && phase<.13f && speed>40) {
                        const auto& s=platforms[p.support];
                        audio.event(s.ground?footSounds[index]:"step-grate",g.crouch>.6f?.28f:.31f+pace*.40f,0,.95f+random()*.10f+campaign.character*.022f);
                        if (pace>.52f) burst({surface.x+foot.anchor,floor},hex(stage->cold),2,32,ParticleKind::smoke);
                    }
                }
                foot.heel={surface.x+foot.anchor,floor};
                const float u=phase/stance;
                foot.angle=.29f*(1-smooth(u/.20f))-.40f*smooth((u-.72f)/.28f);
                foot.planted=true; foot.surface=p.support;
            } else {
                if (foot.planted || changedSurface || oldDirection!=g.direction) {
                    foot.takeoff=foot.heel-Vec{surface.x,surface.y};
                }
                const float u=clamp((phase-stance)/(1-stance));
                const Vec start=Vec{surface.x,surface.y}+foot.takeoff;
                foot.heel={mix(start.x,front,smooth(u)),floor-std::sin(pi*u)*mix(mix(8.f,19.f,g.run),6.f,g.crouch)*g.weight};
                foot.angle=mix(-.40f,.29f,smooth(u));
                foot.planted=false; foot.surface=p.support;
            }
        }
        foot.phase=phase;
    }
}
} // namespace vl
