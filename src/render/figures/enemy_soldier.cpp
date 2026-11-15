#include "render/figures/enemy_view.hpp"
#include "render/figures/humanoid.hpp"
#include "content/enemy_motion.hpp"

namespace vl {
// Soldier built on the shared human rig. The rifle is carried low until a target is in range, and then raised.
// Hits make the body part that was struck react: a head hit snaps the head back, a torso hit rocks the spine,
// a leg hit buckles the knees. A defeated soldier drops the rifle, collapses and topples about the feet.
void Renderer::drawSoldier(const EnemyView& v) {
    const Enemy& e=v.e;
    const Color black=v.black,hull=v.hull,rim=v.rim,plate=v.plate,alert=v.alert,accent=v.accent;
    Humanoid h;
    h.look=Humanoid::Look::soldier;
    h.model=&enemySkeletons[static_cast<size_t>(e.kind)];
    const Vec ground=v.ground;
    const float x=v.x;
    h.ground=ground; h.facing=v.facing; h.stride=v.facing; h.turn=v.turn;
    const float cycle=std::fmod(e.distance/56*h.model->walkCycle,1.f);
    const float pace=e.alive?v.pace:0.f;
    for (int i=0;i<2;++i) {
        const float c=std::fmod(cycle+.5f*i,1.f);
        const bool stance=c<.61f;
        const float u=stance?c/.61f:(c-.61f)/.39f;
        const float xFoot=(stance?mix(15.f,-14.f,u):mix(-14.f,15.f,smooth(u)))*pace*v.facing;
        const float rise=stance?0.f:std::sin(pi*u)*7.f*pace;
        h.heels[i]={x+xFoot+(i==0?-3.5f:3.5f),ground.y-rise};
        const float ang=stance?.26f*(1-smooth(u/.2f))-.34f*smooth((u-.7f)/.3f):mix(-.34f,.26f,smooth(u));
        h.footAngle[i]=ang*pace;
    }
    h.grounded=true;
    h.lean=clamp(e.vx/96*.15f,-.2f,.2f)-e.flinch*e.kick*.05f;
    h.run=pace*.55f; h.weight=pace; h.cycle=cycle;
    h.pelvis=std::cos(cycle*4*pi-.4f)*pace*1.3f;
    if(!e.alive)h.action=ActionPose::death;
    else if(e.flinch>.75f || e.buckle>.8f)h.action=ActionPose::stagger;
    else if(e.flinch>.35f)h.action=ActionPose::hit;
    else if(e.recoil>.1f)h.action=ActionPose::recoil;
    else if(e.warning>0 && e.raise>.2f)h.action=ActionPose::fire;
    else if(e.raise>.2f)h.action=ActionPose::alert;
    else if(std::abs(e.vx)>4.f &&
            std::abs(e.base-(e.origin.y+e.box.h))>8.f)h.action=ActionPose::stairs;
    else if(std::abs(e.vx)>4.f && e.raise>.2f)h.action=ActionPose::chase;
    else if(std::abs(e.vx)>4.f)h.action=ActionPose::patrol;
    else h.action=ActionPose::idle;
    h.actionWeight=e.alive?1.f:std::min(1.f,v.dead/.25f);
    h.time=v.time+e.phase; h.velocity={e.vx,0}; h.landed=0; h.flinch=e.flinch;
    h.headSnap=e.headSnap; h.buckle=e.buckle*.3f;
    h.scarf=false; h.gripFromChest=true; h.farFree=false;
    // The far hand lets go of the rifle while the soldier is walking and hasn't seen the target.
    const float shouldered=smooth(clamp(e.raise));
    h.shoulder=shouldered;
    h.relax=1.f-shouldered;
    if (!e.alive) {
        // Stagger, buckle, topple: the fall builds over half a second.
        h.fall=smooth(v.dead/.5f);
        h.fallDirection=e.fallDir;
        h.flinch=std::max(h.flinch,.7f*(1-h.fall));
    }
    const float lowAngle=v.facing>0?.7f:pi-.7f;
    const Vec chest{x,v.y-12.f};
    const float aimAngle=e.warning>0?e.aim:std::atan2(v.target.y-chest.y,v.target.x-chest.x);
    h.aim=normal(mix(ray(lowAngle),ray(aimAngle),shouldered));
    h.recoil=e.recoil*h.model->recoil;
    h.suit=e.hit>0?hex(0xffe9d0):mix(mix(hex(0x2a323d),accent,.16f),hex(0x14171c),v.burnt*.5f);
    h.shell=e.hit>0?hex(0xffffff):mix(mix(hex(0x56626f),accent,.28f),hex(0x14171c),v.burnt*.5f);
    h.trim=accent; h.edge=mix(hex(0xc4d0d6),accent,.3f); h.shadow=hex(0x1a2029);
    h.ink=black; h.skin=mix(hex(0x39424e),accent,.12f); h.hair=hex(0x20252d);
    h.leather=hex(0x3a342f); h.glove=hex(0x1f242c); h.visor=alert;
    h.weapon=[&](Vec hand,Vec) {
        const Vec aim=h.aim;
        Vec down{-aim.y,aim.x};
        if (down.y<0) down=down*-1.f;
        const Vec stock=hand-aim*13.f, tail=hand-aim*2.f, muzzle=hand+aim*25.f;
        armor(stock,tail,2.6f,2.8f,plate.energy(.8f),rim,true);
        armor(tail,hand+aim*10.f,3.1f,2.8f,hull.energy(.95f),rim,true);
        armor(hand+aim*8.f,muzzle-aim*5.f,2.3f,2.f,hull.energy(.7f),rim,true);
        line(muzzle-aim*5.f,muzzle,2.6f,black);
        line(muzzle-aim*5.f,muzzle,1.6f,rim.energy(.8f));
        polygon(hand+aim*2.f,hand+aim*5.5f,hand+aim*4.f+down*9.f,hand+aim*.5f+down*9.f,black);
        line(hand+aim*1.5f+down*1.5f,hand+aim*3.6f+down*8.2f,1.2f,plate);
        line(hand+aim*4.f-down*3.f,hand+aim*11.f-down*3.f,2.2f,black);
        line(hand+aim*5.f-down*3.f,hand+aim*10.f-down*3.f,1.1f,accent.energy(1.8f));
        line(hand+aim*14.f-down*3.f,hand+aim*14.f-down*5.5f,1.2f,black);
        line(hand-aim*2.f+down*.5f,hand+aim*12.f+down*.5f,.8f,accent.energy(e.warning>0?2.4f:1.2f));
    };
    humanoid(h);
}
} // namespace vl
