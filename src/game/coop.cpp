#include "game/game.hpp"
#include "game/weapon_selection.hpp"
#include "game/detail.hpp"
#include <utility>

namespace vl {
void Game::startCoop(int sector,const Save& loadout,int guestCharacter,bool cooperative) {
    start(sector,loadout,cooperative);
    coop=true;
    auto guest=loadout;
    guest.character=guestCharacter<0?(loadout.character+1)%characters.size():guestCharacter;
    swapOperatives();
    setupLoadout(guest);
    swapOperatives();
    // A shared deployment point, two side-by-side spawn pads. They must choose
    // their own routes once they reach the signposted fork.
    spawnPartner({(stage->insertion.x>0?stage->insertion.x:100.f)+(puzzleMode?48.f:-41.f),
                  stage->insertion.y==0?stage->ground.front().y:stage->insertion.y});
    for (auto& enemy:enemies) {
        enemy.hp=enemy.maxHp=static_cast<int>(std::lround(enemy.maxHp*1.45f));
    }
}

// The movement and weapon routines operate on one operative at a time.
void Game::swapOperatives() {
    controllingPartner=!controllingPartner;
    std::swap(operative,partner);
    std::swap(localInput,partnerInput);
    refreshCharacterBonuses();
}

void Game::releaseBoth() {
    release();
    partnerInput={};
}

void Game::spawnPartner(Vec safe) {
    swapOperatives();
    spawn(safe);
    camera=clamp(safe.x-width*.32f,0,stage->worldWidth-width);
    cameraY=clamp(safe.y-27.f-height*.78f,-560.f,260.f);
    player.flash=.9f;
    swapOperatives();
}

void Game::applyControls(const Controls& controls,bool second,bool pointerMoved) {
    if (second && !coop) return;
    if (second) swapOperatives();
    for (int i=0;i<static_cast<int>(Key::count);++i) {
        const auto key=static_cast<Key>(i);
        if (controls.pressed&keyBit(key)) { hold(key,false); input(key,true); }
        hold(key,(controls.held&keyBit(key))!=0);
    }
    if (controls.weapon>=0 && controls.weapon<static_cast<int>(weaponCount)) select(controls.weapon);
    if (controls.cycle) {
        const int next=nextVisibleWeapon(guns,gun,controls.cycle);
        if (next>=0) select(next);
    }
    if (controls.aimMode==AimMode::pointer) pointAt(controls.aim,pointerMoved && length(controls.aim-mouse)>2.5f);
    else pointerActive=false;
    stick(controls.aim,controls.aimMode==AimMode::stick);
    if (second) swapOperatives();
}

bool Game::atExtraction(const Player& body) const {
    // Require the player to enter the doorway, not just touch its closed shutter.
    return std::abs(body.box.center().x-stage->exit.x)<22.f &&
        std::abs(body.box.bottom()-stage->exit.y)<45.f;
}

void Game::updateCamera(float dt) {
    const Vec center=player.box.center();
    const float maxX=std::max(0.f,stage->worldWidth-width);
    // Only the top and bottom 10% of the viewport enable cursor look-ahead.
    // Return to a player-centered target whenever the pointer leaves those
    // regions. Compute the target every tick, including when aiming stops.
    float verticalLead=0.f;
    float horizontalLead=0.f;
    if (pointerActive) {
        const float cursorY=mouse.y-cameraY;
        const float border=height*.10f;
        float influence=0.f;
        if (cursorY<border) influence=-clamp((border-cursorY)/border);
        else if (cursorY>height-border)
            influence=clamp((cursorY-(height-border))/border);
        verticalLead=height*.23f*influence;
        if (influence!=0.f)
            horizontalLead=clamp((mouse.x-center.x)*.35f,-width*.20f,width*.20f)*std::abs(influence);
    }
    const float targetX=clamp(center.x-width*.5f+horizontalLead,0.f,maxX);
    const float targetY=clamp(center.y-height*.5f+verticalLead,-560.f,260.f);
    // Time-independent exponential damping also smooths the return to center.
    const float blend=1.f-std::exp(-4.2f*std::max(0.f,dt));
    camera=mix(camera,targetX,blend);
    cameraY=mix(cameraY,targetY,blend);
    // Keep the player visible even after a sudden teleport or very fast fall.
    camera=clamp(camera,center.x-width*.77f,center.x-width*.23f);
    camera=clamp(camera,0.f,maxX);
    cameraY=clamp(cameraY,center.y-height*.90f,center.y-height*.10f);
    cameraY=clamp(cameraY,-560.f,260.f);

}
}
