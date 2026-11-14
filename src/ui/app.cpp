#include "ui/app.hpp"
#include "ui/common.hpp"
#include <iomanip>
#include <sstream>

using namespace vl::ui;

namespace vl {
App::App(SDL_Window* win,Renderer& renderer,Audio& mixer,Storage& storage)
    :window(win),r(renderer),audio(mixer),store(storage),game(*this) {
    settings=store.settings();
    input.configure(settings);
    audio.configure(settings);
    audio.track("menu");
    refresh();
}

void App::refresh() { for (int i=0;i<3;++i) slots[i]=store.get(i+1); }

void App::show(View v) {
    view=v; focus=-1; buttons.clear();
    worldFrame=mouseAiming=mouseMoved=false;
    // A new mission or a resumed view starts from a centered camera.
    if (v==View::play) cameraTilt={};
    if (v!=View::multiplayer) SDL_StopTextInput();
    SDL_ShowCursor(v==View::play?0:1);
    if (v!=View::play && v!=View::pause && v!=View::intro && v!=View::failed && v!=View::epilogue) audio.track("menu");
}

void App::inform(std::string message) { notification=std::move(message); notice=4; }

bool App::persist(Save& next,int target) {
    try {
        store.put(target,next); slots[target-1]=next; return true;
    } catch (const std::exception& e) {
        inform(e.what()); return false;
    }
}

void App::commitSettings() {
    audio.configure(settings);
    input.configure(settings);
    try { store.put(settings); }
    catch (const std::exception& e) { inform(e.what()); }
}

void App::changeFullscreen() {
    const U32 flags=(SDL_GetWindowFlags(window)&sdl::fullscreen)?0:sdl::fullscreen;
    if (SDL_SetWindowFullscreen(window,flags)<0) inform(SDL_GetError());
}

void App::ask(View previous,std::string title,std::string text,std::function<void()> action) {
    confirmReturn=previous; confirmTitle=std::move(title); confirmCopy=std::move(text); confirmAction=std::move(action);
    show(View::confirm);
}

void App::closeAction() {
    if (view==View::epilogue) { skipEpilogue(); return; }
    if (coopStarted) {
        if (view==View::play) {
            coopPaused=!coopPaused; input.reset(); mouseAiming=mouseMoved=false;
            SDL_ShowCursor(coopPaused?1:0);
        }
        else if (view==View::failed || view==View::cleared || view==View::coopLoading) leaveCoop();
        return;
    }
    switch (view) {
    case View::multiplayer:
        if (lobby==Lobby::menu) show(View::menu);
        else leaveCoop();
        break;
    case View::menu: running=false; break;
    case View::settings: show(settingsReturn); break;
    case View::controls: rebinding=-1; show(View::settings); break;
    case View::intro: launch(0); break;
    case View::play: game.pause(true); accumulator=0; audio.track(""); show(View::pause); break;
    case View::pause: game.pause(false); audio.track(sectorTrack(game.index)); show(View::play); break;
    case View::armory:
    case View::cleared:
    case View::failed: showMap(); break;
    case View::confirm: show(confirmReturn); break;
    default: show(View::menu); break;
    }
}

void App::navigate(int direction) {
    const int count=static_cast<int>(buttons.size());
    if (count==0) return;
    if (direction>0) focus=(focus+1)%count;
    else focus=focus<0?count-1:(focus+count-1)%count;
}

void App::activate() {
    const int count=static_cast<int>(buttons.size());
    if (count==0) return;
    auto action=buttons[std::clamp(focus,0,count-1)].action; action();
}

void App::movePointer(Vec position) {
    if (view==View::play && !coopPaused) {
        mouseMoved |= !mouseAiming || length(position-mouse)>2.5f;
        mouseAiming=true;
    }
    mouse=position;
}

Vec App::aimPoint() const {
    // Pick through the transform the player saw, including camera interpolation and shake.
    return worldFrame?r.worldAt(mouse):mouse+Vec{game.camera,0};
}

void App::event(const SDL_Event& e) {
    input.event(e);
    if (e.type==sdl::quit) { running=false; return; }
    if (e.type==sdl::window && e.window.event==sdl::focusLost) {
        if (view==View::play && !coopStarted) closeAction();
        if (coopStarted) { mouseAiming=mouseMoved=false; pendingControls={}; }
        return;
    }
    if (e.type==sdl::window && e.window.event==sdl::mouseLeave) {
        mouseAiming=mouseMoved=false;
        mouse={-100,-100};
        if (view==View::play && !coopStarted) game.pointerActive=false;
        return;
    }
    if (view==View::multiplayer && lobbyInput(e)) return;
    if (e.type==sdl::mouseMotion) {
        movePointer(r.mousePosition(e.motion.x,e.motion.y)); focus=-1;
    } else if (e.type==sdl::mouseDown && e.button.button==1) {
        movePointer(r.mousePosition(e.button.x,e.button.y));
        if (view!=View::play || coopPaused)
            for (const auto& b:buttons) if (b.box.contains(mouse)) { auto action=b.action; action(); break; }
    }
    if (e.type==sdl::controllerButton && e.cbutton.state && (view!=View::play || coopPaused)) {
        using namespace sdl::pad;
        const int button=e.cbutton.button;
        if (button==down || button==right) navigate(1);
        else if (button==up || button==left) navigate(-1);
        else if (button==a) activate();
        else if (button==b && view!=View::pause) closeAction();
        return;
    }
    if (e.type!=sdl::keyDown || e.key.repeat) return;
    const int key=e.key.keysym.scancode;
    if (key==sdl::key::f11) { changeFullscreen(); return; }
    if (key==sdl::key::f3) { settings.diagnostics=!settings.diagnostics; input.resetPeak(); commitSettings(); return; }
    if (view==View::controls && rebinding>=0) { captureKey(key); return; }
    // Esc is reserved, so it always works. P and the gamepad's Start button pause through the polled action.
    if (key==sdl::key::esc) { closeAction(); return; }
    if (view==View::play && !coopPaused) return;
    if (view==View::intro && (key==sdl::key::space || key==sdl::key::enter)) { launch(0); return; }
    if (key==sdl::key::down || key==sdl::key::right || key==sdl::key::tab) navigate(1);
    else if (key==sdl::key::up || key==sdl::key::left) navigate(-1);
    else if (key==sdl::key::enter || key==sdl::key::space) activate();
}

void App::update(float dt) {
    clock+=dt; notice=std::max(0.f,notice-dt);
    if (script) script(scriptFrame++);
    input.poll();
    const bool followingMouse=settings.motion && view==View::play && !coopPaused && mouseAiming &&
        !input.aimStickActive();
    // Letterbox margins hold the nearest edge angle until the pointer leaves the window.
    cameraTilt.advance(followingMouse?CameraTilt::target(mouse):Vec{},dt);
    if (view==View::multiplayer || coopStarted) {
        pollCoop(dt);
        if (coopStarted) {
            if (view==View::play && input.pressed(Act::pause)) closeAction();
            if (view==View::epilogue) updateEpilogue(dt);
            else advanceCoop(dt);
            network.flush();
        } else audio.pace(1);
        return;
    }
    if (view==View::intro) {
        film+=dt;
        if (film>=net::openingDuration/1000.f) launch(0);
    }
    if (view==View::epilogue) { updateEpilogue(dt); return; }
    if ((view==View::play || view==View::pause) && input.pressed(Act::pause)) closeAction();
    if (view!=View::play) { audio.pace(1); return; }
    driveGame(dt);
    const float scale=game.advance(dt);
    audio.pace(game.timeScale);
    accumulator+=dt*scale;
    while (accumulator>=step && game.mode==Mode::active) {
        previousPlayer=game.player;
        previousCamera=game.camera;
        game.tick();
        accumulator-=step;
    }
    if (game.mode==Mode::complete || game.mode==Mode::failed) finishMission();
}

void App::draw() {
    buttons.clear();
    switch (view) {
    case View::menu: menu(); break;
    case View::multiplayer: multiplayer(); break;
    case View::create: create(); break;
    case View::load: load(); break;
    case View::campaign: campaign(); break;
    case View::armory: armory(); break;
    case View::settings: options(); break;
    case View::controls: keyBindings(); break;
    case View::credits: credits(); break;
    case View::intro: cinematic(); break;
    case View::coopLoading: coopCinematic(); break;
    case View::epilogue: sectorCinematic(); break;
    case View::play:
    case View::pause:
    case View::failed:
    case View::cleared: {
        const float alpha=coopStarted && !network.hosting()?netInterpolation:
            view==View::play?clamp(accumulator/step):1.f;
        r.drawWorld(game,settings.high,settings.motion,alpha,previousPlayer,previousCamera,
                    game.coop?&previousPartner:nullptr,
                    coopStarted && !network.hosting()?&previousWorld:nullptr,cameraTilt.value());
        worldFrame=true;
        hud();
        if (coopStarted && (coopPaused || view!=View::play)) coopOverlay();
        else if (view!=View::play) overlay();
        break;
    }
    case View::confirm:
        shell("orbital");
        r.gradient({300,170,600,260},hex(0x1a2d38),hex(0x12202b));
        r.outline({300,170,600,260},hex(0x587582));
        r.text({323,239},confirmTitle,39,white,true);
        copy({325,276},confirmCopy,535,18);
        button({325,347,224,43},"CANCEL",[this]{show(confirmReturn);});
        button({648,347,227,43},"CONFIRM",[this]{auto action=std::move(confirmAction); action();},true,true);
        break;
    }
    if (settings.diagnostics && view!=View::coopLoading && view!=View::epilogue) diagnostics();
    if (notice>0) {
        const float w=std::min(1150.f,r.measure(notification,15)+32);
        r.rect({(width-w)*.5f,5,w,29},hex(0x142630).alpha(.97f));
        r.text({(width-w)*.5f+16,25},notification,15,teal);
    }
}
} // namespace vl
