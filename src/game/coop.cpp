#include "game/game.hpp"
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
}

void Game::releaseBoth() {
    release();
    partnerInput={};
}

void Game::spawnPartner(Vec safe) {
    swapOperatives();
    spawn(safe);
    camera=clamp(safe.x-width*.32f,0,stage->worldWidth-width);
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
        for (int hop=1;hop<static_cast<int>(weaponCount);++hop) {
            const int count=static_cast<int>(weaponCount);
            const int next=(gun+controls.cycle*hop+count)%count;
            if (guns[next].available) { select(next); break; }
        }
    }
    if (controls.aimMode==AimMode::pointer) pointAt(controls.aim,pointerMoved && length(controls.aim-mouse)>2.5f);
    else pointerActive=false;
    stick(controls.aim,controls.aimMode==AimMode::stick);
    if (second) swapOperatives();
}

bool Game::atExtraction(const Player& body) const {
    return std::abs(body.box.center().x-stage->exit.x)<48 && std::abs(body.box.bottom()-stage->exit.y)<45;
}

void Game::updateCamera(float dt) {
    const float target=clamp(player.box.x-width*.34f+player.velocity.x*.28f,0,stage->worldWidth-width);
    camera=mix(camera,target,1-std::exp(-4.9f*dt));
}
}
