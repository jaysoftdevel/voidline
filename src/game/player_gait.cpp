#include "game/game.hpp"
#include "game/detail.hpp"
#include "game/running_cycle.hpp"
#include "content/skeleton.hpp"
#include "game/stairs.hpp"

namespace vl {
void Game::updateGait(float dt,float travel,float accel,int priorSupport,float landing) {
    auto& p=player;
    auto& g=p.gait;
    const auto& model=characterSkeletons[static_cast<size_t>(character-characters.data())];
    const float speed=std::abs(p.velocity.x), pace=clamp(speed/(character->speed*speedScale),0,1.28f);
    const float center=p.box.center().x, floor=p.box.bottom();
    const Stair* stair=nullptr;
    if (p.dropThrough==0 && p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f) {
        float best=18.f;
        for (const Stair& s:stage->stairs) if (validStair(s) && center>=s.from.x && center<=s.to.x) {
            const float error=std::abs(stairY(s,center)-floor);
            if (error<=best) { best=error;stair=&s; }
        }
    }
    const bool onGround=p.support>=0 || stair!=nullptr;
    g.crouch=mix(g.crouch,p.crouched?1.f:0.f,1-std::exp(-(p.crouched?29:24)*dt));
    g.run=mix(g.run,smooth((pace-.29f)/.88f)*(1-g.crouch),1-std::exp(-11*dt));
    const float tuckTarget=onGround?0.f:smooth((p.jumpAge-.055f)/.14f)*(1-smooth((p.velocity.y-45)/610));
    g.tuck=mix(g.tuck,tuckTarget,1-std::exp(-14*dt));
    g.reach=mix(g.reach,onGround?0.f:smooth(p.velocity.y/510),1-std::exp(-13*dt));
    const float anticip=p.windup>0?3.4f*(1-p.windup/windupDuration):0.f;
    const float bob=mix(std::cos(g.cycle*4*pi-.4f)*.9f,
                        running::hipOffset(g.cycle),g.run)*g.weight;
    g.pelvis=mix(g.pelvis,bob+anticip,1-std::exp(-28*dt));
    g.weight=mix(g.weight,onGround?smooth(speed/65):0.f,1-std::exp(-14*dt));
    // Gravity and the planted-foot support area limit the plausible torso tilt.
    // In particular, facing forward while backpedaling cannot put the upper
    // body's center of mass far behind the support foot.
    const bool backpedaling=speed>20.f && p.velocity.x*p.facing<0.f;
    const float requestedLean=p.velocity.x/(character->speed*speedScale)*(.13f+g.run*.065f)
        +clamp(accel*.000035f,-.075f,.075f);
    const float balanceLimit=backpedaling?(stair?.095f:.13f):.235f;
    g.lean=mix(g.lean,clamp(requestedLean,-balanceLimit,balanceLimit),1-std::exp(-14*dt));
    if (landing>0) g.compressVelocity+=landing*124;
    g.compressVelocity+=(-161*g.compression-20*g.compressVelocity)*dt;
    g.compression=std::max(0.f,g.compression+g.compressVelocity*dt);
    if (g.compression==0 && g.compressVelocity<0) g.compressVelocity=0;

    const int oldDirection=g.direction;
    if (speed>25) g.direction=sign(p.velocity.x);
    const bool changedSurface=p.support!=priorSupport ||
        (stair && (g.feet[0].surface!=-2 || g.feet[1].surface!=-2));
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
    const float cycleLength=mix(mix(47.f,118.f,g.run),42.f,g.crouch)*model.stride/model.walkCycle;
    if (onGround) g.cycle=std::fmod(g.cycle+std::abs(travel)/cycleLength,1.f);

    for (int i=0;i<2;++i) {
        auto& foot=g.feet[i];
        const float phase=running::unit(g.cycle+.5f*i);
        const auto footStep=running::sample(phase,stance);
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
            const Rect surface=stair?Rect{stair->from.x,stair->from.y,
                stair->to.x-stair->from.x,17.f}:platforms[p.support].box;
            const int currentSurface=stair?-2:p.support;
            const auto atSurface=[&](float x) {return stair?stairY(*stair,x):floor;};
            const float rest=center+(i==0?-5.f:5.f);
            const float reach=cycleLength*stance*.46f*g.weight;
            const float front=clamp(center+reach*g.direction,surface.x+2,surface.right()-2);
            if (speed<13) {
                if (foot.surface!=currentSurface || !foot.planted || std::abs(foot.heel.x-rest)>12) {
                    const float x=clamp(rest,surface.x+2,surface.right()-2);
                    foot.heel=mix(foot.heel,{x,atSurface(x)},1-std::exp(-19*dt));
                    foot.anchor=foot.heel.x-surface.x;
                } else {
                    foot.heel={surface.x+foot.anchor,atSurface(surface.x+foot.anchor)};
                }
                foot.surface=currentSurface; foot.planted=true;
                foot.angle=approach(foot.angle,0,dt*8);
            } else if (footStep.planted) {
                const bool touchdown=!foot.planted || phase<foot.phase || foot.surface!=currentSurface;
                if (touchdown) {
                    // Use the recovering foot's final landing position: no snap
                    // from the airborne arc back to a precomputed stride.
                    const float projected=(!foot.planted || changedSurface)?foot.heel.x:front;
                    foot.anchor=clamp(clamp(projected,center-24,center+24),surface.x+2,surface.right()-2)-surface.x;
                    if (!changedSurface && phase<.13f && speed>40) {
                        const bool groundSurface=!stair && p.support>=0 && platforms[p.support].ground;
                        // Each real foot plant triggers one heel/toe boot strike, with
                        // softer crouching and heavier, quicker running steps.
                        audio.event(groundSurface?footSounds[static_cast<int>(stage->biome)]:"boot-grate",
                            g.crouch>.6f?.19f:.31f+pace*.28f,0,
                            (.91f+random()*.13f+campaign.character*.009f)*(g.crouch>.6f?.89f:1.f));
                        if (pace>.52f) burst({surface.x+foot.anchor,floor},hex(stage->cold),2,32,ParticleKind::smoke);
                    }
                }
                foot.heel={surface.x+foot.anchor,atSurface(surface.x+foot.anchor)};
                foot.angle=footStep.pitch*g.weight;
                foot.planted=true; foot.surface=currentSurface;
            } else {
                if (foot.planted || changedSurface || oldDirection!=g.direction) {
                    foot.takeoff=foot.heel-Vec{surface.x,surface.y};
                }
                const Vec start=Vec{surface.x,surface.y}+foot.takeoff;
                const float next=clamp(mix(start.x,front,footStep.progress),surface.x+2,surface.right()-2);
                foot.heel={next,atSurface(next)-footStep.clearance*
                    mix(mix(8.f,21.f,g.run),6.f,g.crouch)*g.weight};
                foot.angle=footStep.pitch*g.weight;
                foot.planted=false; foot.surface=currentSurface;
            }
        }
        foot.phase=phase;
    }
}
} // namespace vl
