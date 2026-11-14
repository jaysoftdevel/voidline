#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/coop_story.hpp"
#include "ui/sector_story.hpp"
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
    lobby=Lobby::menu; lanPage=0; rosterPreview=false;
    coopCharacter=std::clamp(save.character,0,static_cast<int>(characters.size())-1);
    remoteCharacter=-1; characterLocked=remoteLocked=false;
    openingSkipped=skipRequested=false;
    coopDifficulty=settings.difficulty;
    advertisedAddress=net::Session::localAddress();
    show(View::multiplayer);
}

void App::leaveCoop(const std::string& message) {
    network.close();
    coopStarted=coopPaused=snapshot=false;
    mission=0; epilogueAt=0; pendingControls={}; netSounds.clear();
    game.release();
    input.reset();
    lobby=Lobby::menu;
    characterLocked=remoteLocked=false;
    remoteCharacter=-1; openingSkipped=skipRequested=false;
    SDL_StopTextInput();
    audio.pace(1);
    show(View::multiplayer);
    if (!message.empty()) inform(message);
}

void App::prepareCoop() {
    coopStarted=true; coopPaused=snapshot=false;
    pendingControls={}; netSounds.clear();
    previousPlayer=game.player; previousPartner=game.teammate().player;
    previousCamera=game.camera; previousWorld.capture(game);
    accumulator=netFrame=film=0; netInterpolation=1; notice=0;
    simulationTick=lastSnapshotTick=0;
    preloadStep=0; probes=0;
    guestLoaded=scheduleConfirmed=loadingMusic=openingSkipped=skipRequested=false;
    hostClock={};
    preparedAt=net::now(); filmAt=deployAt=probeAt=lastRemoteInput=guestLead=epilogueAt=0;
    input.reset();
    audio.pace(1);
    audio.track("");
    show(View::coopLoading);
}

void App::startCoop(int index,bool retry) {
    using State=net::Session::State;
    if (!network.hosting() || network.state()!=State::connected ||
        !characterLocked || !remoteLocked || remoteCharacter<0) return;
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
        loadout.character=coopCharacter;
        game.startCoop(index,loadout,remoteCharacter);
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

    if (kind==Packet::operative && mission==0 && network.state()==net::Session::State::connected) {
        valid=body.size()==1 && body[0]<characters.size() && !remoteLocked;
        if (valid) {
            remoteCharacter=body[0]; remoteLocked=true;
            if (network.hosting() && characterLocked) startCoop(0);
        }
        if (!valid) network.fail("Invalid or repeated operative selection.");
        return;
    }
    if (network.hosting()) {
        if (kind==Packet::skipRequest && body.empty() && mission==1 && view==View::coopLoading) {
            skipRequested=true; openingSkipped=true; return;
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
        if (kind==Packet::skipOpening && mission==1 && view==View::coopLoading) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission &&
                  timing.first>filmAt && timing.first<=hostClock.estimate(local)+5000 &&
                  timing.second==0;
            if (valid) {
                deployAt=timing.first;
                openingSkipped=true;
                loadingMusic=true;
                audio.track("");
            }
            if (!valid) network.fail("Invalid prologue skip timing.");
            return;
        }
        if (kind==Packet::epilogue && coopStarted && view==View::epilogue && !epilogueAt) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission && timing.first>0 && timing.second==0;
            if (valid) epilogueAt=timing.first;
        }
        if (kind==Packet::epilogueDone && coopStarted && (view==View::epilogue || view==View::cleared)) {
            valid=net::decodeTiming(body,timing) && timing.mission==mission && timing.first==0 && timing.second==0;
            if (valid) show(View::cleared);
        }
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
                loadout.character=remoteCharacter;
                valid=characterLocked && remoteLocked && remoteCharacter>=0;
                if (!valid) { network.fail("Missing operative lock-in before deployment."); return; }
                game.startCoop(index,loadout,coopCharacter);
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
    const size_t total=artwork+story::missionPreloads;
    if (preloadStep<total) {
        try {
            if (preloadStep<artwork) r.preload(story::opening[preloadStep].art);
            else switch (preloadStep-artwork) {
            case 0: r.preload(game.stage->art); break;
            case 1: r.preload("carbine"); break;
            case 2: if (mission==1) audio.preload("coop-intro"); break;
            case 3: audio.preload(sectorTrack(game.index)); break;
            case 4: r.preload(story::scenery(story::chapters[game.index].set)); break;
            case 5: audio.preload(story::track(game.index)); break;
            case 6: r.preload("cinematic/deck"); break;
            case 7: r.preload("cinematic/aster"); break;
            case 8: r.preload("cinematic/recorder"); break;
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
        if (skipRequested && network.hosting() && scheduleConfirmed) {
            const auto earlier=net::now()+950;
            if (earlier<deployAt) {
                deployAt=earlier;
                network.send(net::Packet::skipOpening,net::encodeTiming({mission,deployAt,0}));
            }
            skipRequested=false;
            openingSkipped=true;
            loadingMusic=true;
            audio.track("");
        }
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
    } else if (mouseAiming) {
        result.aimMode=AimMode::pointer;
        result.aim=aimPoint();
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
    game.applyControls(commands,false,mouseMoved);
    mouseMoved=false;
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
    if (game.mode==Mode::complete) beginEpilogue();
    else show(View::failed);
}
}

namespace vl {
void App::lockOperative() {
    if (view!=View::multiplayer || network.state()!=net::Session::State::connected ||
        mission!=0 || characterLocked || coopCharacter<0 || coopCharacter>=static_cast<int>(characters.size())) return;
    const uint8_t choice=static_cast<uint8_t>(coopCharacter);
    if (!network.send(net::Packet::operative,std::span<const uint8_t>(&choice,1))) return;
    characterLocked=true;
    if (network.hosting() && remoteLocked) startCoop(0);
}
void App::skipPrologue() {
    if (view==View::intro) { launch(0); return; }
    if (view!=View::coopLoading || mission!=1 || openingSkipped) return;
    openingSkipped=true;
    if (network.hosting()) skipRequested=true;
    else if (!network.send(net::Packet::skipRequest))
        leaveCoop("Could not send the prologue skip request.");
}
}
