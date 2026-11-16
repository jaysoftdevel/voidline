#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::jumpFrom(int support,int takeoffDirection) {
    auto& p=player;
    const Vec lift=support>=0?platforms[support].velocity:Vec{};
    p.jumpBuffer=0; p.coyote=0; p.jumpAge=.001f; p.windup=0;
    p.velocity.y=-(character->jump*jumpScale)+clamp(lift.y,-45,45);
    // A standing operative can jump horizontally by pressing Jump + Left/Right
    // together. The foot push supplies the launch impulse before leaving ground;
    // once airborne, directional keys are ignored by runHorizontal().
    // Preserve established running momentum instead of snapping or reversing it.
    const float groundSpeed=character->speed*speedScale*(p.sprinting?1.28f:1.f);
    if (takeoffDirection!=0 && p.velocity.x*takeoffDirection>=0.f)
        p.velocity.x=takeoffDirection*std::max(std::abs(p.velocity.x),groundSpeed*.78f);
    p.velocity.x+=clamp(lift.x,-75,75);
    p.jumpTakeoffDirection=0;
    p.support=-1;
    const auto& feet=p.gait.feet;
    const int push=(feet[0].heel.x-feet[1].heel.x)*p.gait.direction<0?0:1;
    p.gait.launchFoot=float(push);
    audio.event("jump",p.sprinting?.80f:.68f,0,.98f+campaign.character*.006f);
    burst(feet[push].heel,hex(stage->cold),p.sprinting?10:6,75,ParticleKind::smoke);
}

// One acrobatic flip per airtime, a little weaker than the ground jump.
void Game::airFlip(int direction) {
    auto& p=player;
    p.airJumpUsed=true; p.jumpBuffer=0; p.jumpAge=.001f;
    p.velocity.y=-(character->jump*jumpScale)*.82f;
    p.flip=flipSeconds; p.flipDirection=p.facing>=0?1.f:-1.f;
    (void)direction; // The player cannot redirect their center of mass in mid-air.
    // A flip in mid-air has no boot push-off: use a light fabric/air sound.
    audio.event("jump-air",.62f,0,1.f);
    burst({p.box.center().x,p.box.bottom()-6},hex(stage->cold),9,92,ParticleKind::smoke);
    ring({p.box.center().x,p.box.bottom()-4},hex(stage->cold),34,.32f);
    message({p.box.center().x,p.box.y-14},"FLIP",hex(0xffe27a),.55f);
}

// Ground jump with a short wind-up, coyote time for late presses and the single mid-air flip.
void Game::handleJump(bool grounded,int priorSupport,int direction,float dt) {
    auto& p=player;
    if (p.windup>0) {
        // Direction is chosen on the last grounded frame, not latched on the
        // initial keypress. Once airborne no steering forces are applied.
        p.jumpTakeoffDirection=direction;
        p.windup-=dt;
        if (p.windup<=0 && (grounded || p.coyote>0)) jumpFrom(priorSupport,p.jumpTakeoffDirection);
    } else if (p.jumpBuffer>0 && p.coyote>0 && !p.crouched) {
        if (grounded) { p.windup=windupDuration; p.jumpBuffer=0; p.jumpTakeoffDirection=direction; }
        else jumpFrom(-1,0); // Coyote jump preserves airborne momentum without a second side push.
    } else if (p.jumpBuffer>0 && !grounded && !p.airJumpUsed && !p.crouched && p.windup<=0) {
        airFlip(direction);
    }
}
} // namespace vl
