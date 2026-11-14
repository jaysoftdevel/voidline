#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/coop_story.hpp"
#include <exception>

using namespace vl::ui;

namespace vl {
namespace {
constexpr int clockSamples=5;
constexpr int stepsPerSnapshot=2;
}

void App::event(std::string_view name,float gain,float pan,float rate) {
    audio.event(name,gain,pan,rate);
    if (coopStarted && view==View::play && network.hosting() && netSounds.size()<32)
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
    audio.pace(1);
    show(View::multiplayer);
    if (!message.empty()) inform(message);
}

void App::prepareCoop() {
    coopStarted=true; coopPaused=coopMouse=snapshot=false;
    pendingControls={}; netSounds.clear();
    previousPlayer=game.player; previousPartner=game.teammate().player;
    previousCamera=game.camera; previousWorld.capture(game);
    accumulator=netFrame=film=0; netInterpolation=1; notice=0;
    simulationTick=lastSnapshotTick=0;
    preloadStep=0; probes=0;
    guestLoaded=scheduleConfirmed=loadingMusic=false;
    hostClock={};
    preparedAt=net::now(); filmAt=deployAt=probeAt=lastRemoteInput=guestLead=0;
    input.reset();
    audio.pace(1);
    audio.track("");
    show(View::coopLoading);
}

void App::startCoop(int index,bool retry) {
    using State=net::Session::State;
    if (!network.hosting() || network.state()!=State::connected) return;
    const bool valid=mission==0?(index==0 && !retry):
        retry?(view==View::failed && index==game.index):
              (view==View::cleared && index==game.index+1 && index<sectorCount);
    if (!valid) return;
    ++mission;
    if (retry) game.retry();
    else {
        Save loadout;
        loadout.difficulty=coopDifficulty;
        loadout.cleared=index;
        game.startCoop(index,loadout);
    }
    if (!network.send(net::Packet::prepare,net::encodeMission(game.index,coopDifficulty,mission))) return;
    prepareCoop();
}

void App::probeHost() {
    probeAt=net::now();
    network.send(net::Packet::clockProbe,net::encodeTiming({mission,probeAt,0}));
}

void App::receiveCoop(std::span<const uint8_t> data) {
    using net::Packet;
    const auto kind=static_cast<Packet>(data.front());
    const auto body=data.subspan(1);
    const auto local=net::now();
    net::Timing timing{};
    bool valid=false;

    if (network.hosting()) {
        if (kind==Packet::ready && mission==0 && body.empty()) {
            startCoop(0);
            return;
        }
        if (kind==Packet::controls && coopStarted) {
            Controls controls;
            uint32_t generation=0;
            valid=net::decodeControls(body,controls,generation) && generation<=mission;
            if (valid && generation==mission && view==View::play && game.mode==Mode::active) {
                game.applyControls(controls,true);
                lastRemoteInput=local;
            }
        }
        if (view==View::coopLoading && kind==Packet::clockProbe) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission && timing.second==0;
            if (valid) network.send(Packet::clockReply,net::encodeTiming({mission,timing.first,local}));
        }
        if (view==View::coopLoading && kind==Packet::loaded) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission && timing.first>=350 &&
                  timing.first<=5000 && timing.second==0 && !guestLoaded;
            if (valid) {
                guestLoaded=true;
                guestLead=timing.first;
            }
        }
        if (view==View::coopLoading && kind==Packet::scheduled) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission && deployAt &&
                  timing.first==filmAt && timing.second==deployAt;
            if (valid) scheduleConfirmed=true;
        }
    } else {
        if (kind==Packet::prepare) {
            int index=0,level=0;
            uint32_t generation=0;
            valid=net::decodeMission(body,index,level,generation) && generation==mission+1 &&
                (mission==0?index==0:
                 (view==View::failed && index==game.index) || (view==View::cleared && index==game.index+1));
            if (valid) {
                mission=generation;
                Save loadout;
                loadout.difficulty=level; loadout.cleared=index;
                game.startCoop(index,loadout);
                prepareCoop();
                probeHost();
            }
        }
        if (view==View::coopLoading && kind==Packet::clockReply) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission &&
                  timing.first==probeAt && probes<clockSamples &&
                  hostClock.sample(timing.first,local,timing.second);
            if (valid && ++probes<clockSamples) probeHost();
        }
        if (view==View::coopLoading && kind==Packet::schedule) {
            valid=guestLoaded && !deployAt && net::decodeTiming(body,timing) && timing.mission==mission &&
                  timing.second>=timing.first && timing.second-timing.first==
                    (mission==1?net::openingDuration:net::transitionDuration) &&
                  timing.second>hostClock.estimate(local);
            if (valid) {
                filmAt=timing.first; deployAt=timing.second;
                scheduleConfirmed=network.send(Packet::scheduled,body);
            }
        }
        if (kind==Packet::world && coopStarted && deployAt) {
            previousPlayer=game.player; previousPartner=game.teammate().player;
            previousCamera=game.camera;
            previousWorld.capture(game);
            std::vector<net::Sound> sounds;
            net::WorldFrame frame;
            valid=net::decodeWorld(body,game,sounds,&frame) && frame.mission==mission &&
                  (!snapshot || frame.tick>simulationTick);
            if (valid) {
                const bool continuous=snapshot && frame.tick-simulationTick<=stepsPerSnapshot*3;
                simulationTick=frame.tick;
                netInterpolation=continuous?0.f:1.f;
                snapshot=true;
                if (view==View::coopLoading && scheduleConfirmed && hostClock.estimate(local)>=deployAt) deployCoop();
                for (const auto& sound:sounds) audio.event(sound.name,sound.gain,sound.pan,sound.rate);
                showCoopOutcome();
            }
        }
    }
    if (!valid) network.fail("The other player sent an incompatible multiplayer message.");
}

void App::pollCoop(float dt) {
    if (coopStarted && !network.hosting()) netInterpolation=clamp(netInterpolation+dt/net::snapshotInterval);
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

void App::loadCoop() {
    const auto local=net::now();
    const auto deadline=deployAt?deployAt+net::preparationTimeout:preparedAt+net::preparationTimeout;
    const auto hostTime=network.hosting() || !hostClock.ready()?local:hostClock.estimate(local);
    if ((deployAt?hostTime:local)>=deadline) {
        leaveCoop("Co-op deployment timed out while waiting for your partner.");
        return;
    }

    const size_t artwork=(mission==1?story::opening.size():0);
    const size_t total=artwork+4;
    if (preloadStep<total) {
        try {
            if (preloadStep<artwork) r.preload(story::opening[preloadStep].art);
            else switch (preloadStep-artwork) {
            case 0: r.preload(game.stage->art); break;
            case 1: r.preload("carbine"); break;
            case 2: if (mission==1) audio.preload("coop-intro"); break;
            case 3: audio.preload(sectorTrack(game.index)); break;
            }
            ++preloadStep;
        } catch (const std::exception& e) {
            leaveCoop(std::string("Could not load co-op: ")+e.what());
        }
        return;
    }
    if (!network.hosting() && probes==clockSamples && !guestLoaded) {
        guestLoaded=network.send(net::Packet::loaded,net::encodeTiming({mission,hostClock.lead(),0}));
    }
    if (network.hosting() && guestLoaded && !deployAt) {
        filmAt=local+guestLead;
        deployAt=filmAt+(mission==1?net::openingDuration:net::transitionDuration);
        network.send(net::Packet::schedule,net::encodeTiming({mission,filmAt,deployAt}));
    }
    if (!filmAt || !scheduleConfirmed) return;
    if (mission==1 && hostTime>=filmAt) {
        film=std::min(net::openingDuration,hostTime-filmAt)/1000.f;
        if (!loadingMusic) {
            audio.track("coop-intro",false,film);
            loadingMusic=true;
        }
    }
    if (hostTime<deployAt || (!network.hosting() && !snapshot)) return;
    deployCoop();
    if (network.hosting()) sendWorld();
    showCoopOutcome();
}

void App::deployCoop() {
    previousPlayer=game.player; previousPartner=game.teammate().player;
    previousCamera=game.camera; previousWorld.capture(game);
    netInterpolation=1; accumulator=netFrame=0;
    lastRemoteInput=net::now();
    input.reset();
    audio.track(sectorTrack(game.index));
    audio.event("launch",.65f);
    show(View::play);
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
    network.send(net::Packet::world,net::encodeWorld(game,netSounds,{mission,simulationTick}));
    netSounds.clear();
}

void App::advanceCoop(float dt) {
    if (view==View::coopLoading) {
        loadCoop();
        return;
    }
    audio.pace(game.timeScale);
    if (view!=View::play || game.mode!=Mode::active || network.state()!=net::Session::State::connected) return;
    netFrame+=dt;
    const auto commands=readCoopControls();
    if (!network.hosting()) {
        pendingControls.held=commands.held;
        pendingControls.pressed|=commands.pressed;
        if (commands.weapon>=0) pendingControls.weapon=commands.weapon;
        if (commands.cycle) pendingControls.cycle=commands.cycle;
        pendingControls.aimMode=commands.aimMode;
        pendingControls.aim=commands.aim;
        if (netFrame>=net::inputInterval || pendingControls.pressed || pendingControls.weapon>=0 || pendingControls.cycle) {
            network.send(net::Packet::controls,net::encodeControls(pendingControls,mission));
            pendingControls={}; netFrame=0;
        }
        return;
    }
    game.applyControls(commands);
    if (net::now()-lastRemoteInput>=250) game.applyControls({},true);
    accumulator+=dt*game.advance(dt);
    while (accumulator>=step && game.mode==Mode::active) {
        previousPlayer=game.player; previousPartner=game.teammate().player;
        previousCamera=game.camera;
        game.tick();
        ++simulationTick;
        accumulator-=step;
    }
    const bool final=game.mode!=Mode::active;
    if (netFrame>=net::snapshotInterval || final) {
        // When TCP is backed up, keep the latest state instead of queueing obsolete frames.
        if (simulationTick!=lastSnapshotTick && (final || !network.pendingOutput())) {
            sendWorld();
            lastSnapshotTick=simulationTick;
        }
        netFrame=std::fmod(netFrame,net::snapshotInterval);
    }
    showCoopOutcome();
}

void App::showCoopOutcome() {
    if (view!=View::play || (game.mode!=Mode::complete && game.mode!=Mode::failed)) return;
    coopPaused=false;
    audio.event(game.mode==Mode::complete?"complete":"failure");
    show(game.mode==Mode::complete?View::cleared:View::failed);
}
}
