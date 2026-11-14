#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
namespace {
constexpr float snapshotInterval=1.f/30;
constexpr float inputInterval=1.f/60;
}

void App::event(std::string_view name,float gain,float pan,float rate) {
    audio.event(name,gain,pan,rate);
    if (coopStarted && network.hosting() && netSounds.size()<32)
        netSounds.push_back({std::string(name),clamp(gain,0,4),clamp(pan,-1,1),clamp(rate,.06f,4)});
}

void App::openMultiplayer() {
    network.close();
    lobby=Lobby::menu;
    coopDifficulty=settings.difficulty;
    advertisedAddress=net::Session::localAddress();
    show(View::multiplayer);
}

void App::leaveCoop(const std::string& message) {
    network.close();
    coopStarted=coopPaused=coopMouse=snapshot=false;
    mission=0; pendingControls={}; netSounds.clear();
    game.release();
    input.reset();
    lobby=Lobby::menu;
    SDL_StopTextInput();
    show(View::multiplayer);
    if (!message.empty()) inform(message);
}

void App::prepareCoop() {
    coopStarted=true; coopPaused=coopMouse=false;
    pendingControls={}; netSounds.clear(); snapshot=false;
    previousPlayer=game.player; previousPartner=game.teammate().player;
    previousCamera=game.camera;
    accumulator=netFrame=0; netInterpolation=1; notice=0;
    input.reset();
    audio.track(sectorTrack(game.index));
    audio.event("launch",.65f);
    show(View::play);
}

void App::startCoop(int index,bool retry) {
    if (!network.hosting() || network.state()!=net::Session::State::connected) return;
    ++mission;
    if (retry) game.retry();
    else {
        Save loadout;
        loadout.difficulty=coopDifficulty;
        loadout.cleared=index;
        game.startCoop(index,loadout);
    }
    if (!network.send(net::Packet::start,net::encodeMission(game.index,coopDifficulty,mission))) return;
    prepareCoop();
    sendWorld();
}

void App::receiveCoop(std::span<const uint8_t> data) {
    const auto kind=static_cast<net::Packet>(data.front());
    const auto body=data.subspan(1);
    bool valid=false;
    if (network.hosting()) {
        if (kind==net::Packet::ready && !coopStarted && body.empty()) {
            startCoop(coopSector);
            return;
        }
        if (kind==net::Packet::controls && coopStarted) {
            Controls controls;
            uint32_t generation=0;
            valid=net::decodeControls(body,controls,generation);
            if (valid && generation==mission && game.mode==Mode::active)
                game.applyControls(controls,true);
        }
    } else {
        if (kind==net::Packet::start) {
            int index=0,level=0;
            uint32_t generation=0;
            valid=net::decodeMission(body,index,level,generation) && generation>mission;
            if (valid) {
                mission=generation;
                Save loadout;
                loadout.difficulty=level; loadout.cleared=index;
                game.startCoop(index,loadout);
                prepareCoop();
            }
        }
        if (kind==net::Packet::world && coopStarted) {
            const Mode before=game.mode;
            previousPlayer=game.player;
            previousPartner=game.teammate().player;
            previousCamera=game.camera;
            std::vector<net::Sound> sounds;
            valid=net::decodeWorld(body,game,sounds);
            if (valid) {
                netInterpolation=snapshot?0.f:1.f;
                snapshot=true;
                for (const auto& sound:sounds) audio.event(sound.name,sound.gain,sound.pan,sound.rate);
                if (game.mode!=before && game.mode!=Mode::active) {
                    coopPaused=false;
                    audio.event(game.mode==Mode::complete?"complete":"failure");
                    show(game.mode==Mode::complete?View::cleared:View::failed);
                }
            }
        }
    }
    if (!valid) network.fail("The other player sent an incompatible multiplayer message.");
}

void App::pollCoop(float dt) {
    if (coopStarted && !network.hosting()) netInterpolation=clamp(netInterpolation+dt/snapshotInterval);
    const auto prior=network.state();
    network.poll();
    if (network.state()==net::Session::State::connected && prior!=network.state() && !network.hosting())
        network.send(net::Packet::ready);
    for (const auto& data:network.receive()) {
        receiveCoop(data);
        if (network.state()==net::Session::State::idle) break;
    }
    if (coopStarted && network.state()==net::Session::State::idle) {
        const auto reason=network.error();
        leaveCoop(reason.empty()?"The multiplayer session ended.":reason);
    } else if (network.state()==net::Session::State::idle && prior!=network.state() && lobby==Lobby::join) {
        SDL_StartTextInput();
    }
}

Controls App::readCoopControls() const {
    Controls result;
    if (coopPaused || view!=View::play || !(SDL_GetWindowFlags(window)&sdl::inputFocus)) return result;
    const auto map=[&](Key key,Act act) {
        if (input.down(act)) result.held|=keyBit(key);
        if (input.pressed(act)) result.pressed|=keyBit(key);
    };
    map(Key::left,Act::left); map(Key::right,Act::right); map(Key::jump,Act::jump);
    map(Key::fire,Act::fire); map(Key::automatic,Act::assist); map(Key::sprint,Act::sprint);
    map(Key::crouch,Act::crouch); map(Key::slow,Act::slow);
    if (input.pressed(Act::reload)) result.pressed|=keyBit(Key::reload);
    for (int i=0;i<static_cast<int>(weaponCount);++i)
        if (input.pressed(static_cast<Act>(static_cast<int>(Act::weapon1)+i))) result.weapon=i;
    result.cycle=int(input.pressed(Act::next))-int(input.pressed(Act::previous));
    if (input.aimStickActive()) {
        result.aimMode=AimMode::stick; result.aim=input.aimStick();
    } else if (coopMouse) {
        result.aimMode=AimMode::pointer;
        result.aim=mouse+Vec{network.hosting()?mix(previousCamera,game.camera,clamp(accumulator/step)):
                                             mix(previousCamera,game.camera,netInterpolation),0};
    }
    return result;
}

void App::sendWorld() {
    network.send(net::Packet::world,net::encodeWorld(game,netSounds));
    netSounds.clear();
}

void App::advanceCoop(float dt) {
    audio.pace(game.timeScale);
    if (game.mode!=Mode::active || network.state()!=net::Session::State::connected) return;
    netFrame+=dt;
    auto commands=readCoopControls();
    if (!network.hosting()) {
        pendingControls.held=commands.held;
        pendingControls.pressed|=commands.pressed;
        if (commands.weapon>=0) pendingControls.weapon=commands.weapon;
        if (commands.cycle) pendingControls.cycle=commands.cycle;
        pendingControls.aimMode=commands.aimMode;
        pendingControls.aim=commands.aim;
        if (netFrame>=inputInterval || pendingControls.pressed || pendingControls.weapon>=0 || pendingControls.cycle) {
            network.send(net::Packet::controls,net::encodeControls(pendingControls,mission));
            pendingControls={}; netFrame=0;
        }
        return;
    }
    game.applyControls(commands);
    accumulator+=dt*game.advance(dt);
    while (accumulator>=step && game.mode==Mode::active) {
        previousPlayer=game.player; previousPartner=game.teammate().player;
        previousCamera=game.camera;
        game.tick();
        accumulator-=step;
    }
    if (netFrame>=snapshotInterval || game.mode!=Mode::active) {
        sendWorld();
        netFrame=std::fmod(netFrame,snapshotInterval);
    }
    if (game.mode==Mode::complete || game.mode==Mode::failed) {
        coopPaused=false;
        audio.event(game.mode==Mode::complete?"complete":"failure");
        show(game.mode==Mode::complete?View::cleared:View::failed);
    }
}
}
