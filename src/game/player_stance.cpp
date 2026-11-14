#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// Crouching, and standing back up. Jumping from a crouch stands up and jumps in one motion, so the
// order in which the crouch, move and jump keys are pressed never matters.
void Game::updateStance(bool grounded) {
    auto& p=player;
    const bool crouchHeld=held(Key::crouch);
    if (!crouchHeld) p.crouchLatch=false;
    const bool jumpRequested=p.jumpBuffer>0 || p.windup>0;
    if (p.crouched && jumpRequested && headroom(p.box.x,p.box.bottom())) p.crouchLatch=true;
    const bool wantsCrouch=crouchHeld && !p.crouchLatch;
    const bool shouldCrouch=(grounded && wantsCrouch) ||
        (p.crouched && (wantsCrouch || !headroom(p.box.x,p.box.bottom())));
    if (p.crouched==shouldCrouch) return;
    const float feet=p.box.bottom();
    p.crouched=shouldCrouch;
    p.box.h=p.crouched?crouchHeight:playerH;
    p.box.y=feet-p.box.h;
    if (grounded) audio.event("cloth",p.crouched?.58f:.42f,0,p.crouched?.91f:1.09f);
}

void Game::tickPlayerTimers(float dt,bool grounded) {
    auto& p=player;
    for (float* timer:{&p.flash,&p.muzzle,&p.fireDelay,&p.fireBuffer,&p.jumpBuffer})
        *timer=std::max(0.f,*timer-dt);
    p.coyote=grounded?.095f:std::max(0.f,p.coyote-dt);
    p.flip=std::max(0.f,p.flip-dt);
    p.turn=approach(p.turn,float(p.facing),dt*9.5f);
    p.landed=std::max(0.f,p.landed-dt*4.f);
    const bool aimingNow=p.aiming || guns[gun].reload>0 || stickActive;
    p.readyTimer=aimingNow?readyHold:std::max(0.f,p.readyTimer-dt);
    p.ready=approach(p.ready,p.readyTimer>0?1.f:0.f,dt*(p.readyTimer>0?9.f:2.4f));
    // Without room to stand a jump is not possible, and a stale request must not fire later.
    if (p.crouched) { p.windup=0; p.jumpBuffer=0; }
}
} // namespace vl
