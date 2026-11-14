#include "ui/app.hpp"
#include "game/detail.hpp"
#include "ui/common.hpp"
#include "ui/sector_story.hpp"
#include "ui/coop_story.hpp"
#include <cstdlib>

using namespace vl::ui;

namespace vl {
void App::sceneForTest(std::string_view scene) {
    if (scene=="map") {
        save.cleared=sectorCount-1; sector=sectorCount-1; showMap(); return;
    }
    if (scene=="menu") { show(View::menu); return; }
    if (scene=="settings-fps") {
        settings.fpsCounter=true; settings.fpsCorner=1;
        settingsReturn=View::menu; show(View::settings); return;
    }
    if (scene=="fps-game") {
        settings.fpsCounter=true; settings.fpsCorner=3;
        launch(0); game.entry=0; return;
    }
    if (scene=="splash") { show(View::splash); startup=0; return; }
    if (scene.rfind("operative",0)==0) {
        const int chosen=scene.size()>9?std::clamp(int(scene[9]-'0'),0,5):0;
        save.character=chosen; save.cleared=3;
        launch(0); game.entry=0; game.enemies.clear(); game.camera=0;
        return;
    }
    if (scene=="summary") {
        launch(4); game.score=4720; game.kills=31; game.time=387.4f;
        show(View::cleared); return;
    }
    if (scene=="multiplayer-roster") {
        openMultiplayer(); lobby=Lobby::host;
        coopCharacter=5; remoteCharacter=3; remoteLocked=true; rosterPreview=true;
        network.host("127.0.0.1",0);
        return;
    }
    if (scene.rfind("prologue-",0)==0) {
        const int shot=std::clamp(std::atoi(std::string(scene.substr(9)).c_str()),0,6);
        film=story::opening[shot].at+.6f;
        show(View::intro);
        return;
    }
    if (scene=="multiplayer-join") {
        openMultiplayer(); lobby=Lobby::join; return;
    }
    if (scene=="multiplayer-host") {
        openMultiplayer(); lobby=Lobby::host;
        network.host("127.0.0.1",0); return;
    }
    if (scene=="cooperative-menu") {
        openMultiplayer(); lobby=Lobby::host; coopMode=CoopMode::cooperative; return;
    }
    if (scene=="cooperative-insertion") {
        save.character=3; save.cleared=5;
        game.startCoop(0,save,5,true);
        coopMode=CoopMode::cooperative;
        game.entry=0;
        game.camera=0;
        show(View::play);
        for (auto& enemy:game.enemies) enemy.damage=0;
        return;
    }
    if (scene=="cooperative-gate") {
        save.character=3; save.cleared=5;
        game.startCoop(0,save,5,true);
        coopMode=CoopMode::cooperative;
        game.entry=0;
        game.camera=0;
        game.player.box={498,326-56,32,56};
        game.player.support=1;
        game.updateRelays(.8f);
        show(View::play);
        for (auto& enemy:game.enemies) enemy.damage=0;
        return;
    }
    if (scene=="create") { newGame(); return; }
    if (scene=="controls") {
        settings.diagnostics=false;
        show(View::controls);
        // Several keys held at once, to show the live key list.
        for (int k:{sdl::key::d,sdl::key::space,sdl::key::shiftLeft,sdl::key::f,sdl::key::q}) input.inject(k,true);
        return;
    }
    if (scene.rfind("armory",0)==0) {
        save.cleared=5; save.credits=900; showArmory();
        if (scene.size()>6) weapon=scene[6]-'0';
        return;
    }
    if (scene=="intro") { film=5; show(View::intro); return; }
    // Deterministic render checks for all three cinematic layers and prop depth.
    // cinematic-close / cinematic-detail inspect arm/prop overlap; cinematic-recover
    // inspects the capsule, and cinematic-arrival the planted-foot walking cycle.
    if (scene.rfind("cinematic-",0)==0) {
        const bool recover=scene=="cinematic-recover";
        save.character=recover?1:0;
        launch(recover?7:1);
        const float at=scene=="cinematic-establish"?1.4f:
            scene=="cinematic-arrival"?5.8f:
            scene=="cinematic-close"?12.0f:
            scene=="cinematic-detail"?17.8f:
            scene=="cinematic-recover"?12.0f:
            scene=="cinematic-aftermath"?28.3f:23.0f;
        film=at;
        show(View::epilogue);
        return;
    }
    // Scripted gameplay scenes exercise the input path with simulated key chords.
    auto prepare=[&](int index,int gun) {
        save.cleared=3; save.equipped=gun;
        launch(index); game.entry=0;
    };
    if (scene.rfind("stand",0)==0) {
        prepare(0,scene.size()>5?scene[5]-'0':0);
        game.pointer({700,420});
        return;
    }
    if (scene=="enemy-roster") {
        save.cleared=19; save.equipped=0;
        launch(0); game.entry=0; game.camera=0;
        game.player.box.x=92; game.player.box.y=505-game.player.box.h;
        constexpr EnemyKind kinds[]{EnemyKind::scavenger,EnemyKind::sniper,EnemyKind::bulwark,
                                    EnemyKind::interceptor,EnemyKind::juggernaut};
        for (size_t i=0;i<game.enemies.size();++i) {
            auto& e=game.enemies[i];
            if (i>=5) { e.alive=false; continue; }
            const auto& size=spec(kinds[i]);
            const float x=245.f+i*186.f;
            const float y=kinds[i]==EnemyKind::interceptor?407.f:505.f-size.h;
            e.kind=kinds[i]; e.box={x,y,size.w,size.h};
            e.origin={x,y}; e.base=505; e.patrol=0; e.speed=0; e.damage=0;
            e.reach=0; e.facing=-1; e.turn=-1; e.warning=0; e.timer=999;
            e.hp=e.maxHp=size.hp;
        }
        return;
    }
    if (scene.rfind("gallery",0)==0) {
        // Every enemy kind lined up beside the operative, used to inspect animation and detail.
        save.character=scene.size()>7?scene[7]-'0':0;
        prepare(0,0);
        game.pointer({640,420});
        constexpr EnemyKind kinds[]{EnemyKind::ranger,EnemyKind::stalker,EnemyKind::turret,EnemyKind::drone,EnemyKind::ranger};
        constexpr Vec sizes[]{{35,54},{54,37},{48,46},{48,30},{35,54}};
        for (size_t i=0;i<game.enemies.size();++i) {
            auto& e=game.enemies[i];
            if (i<5) {
                e.kind=kinds[i];
                const float px=250.f+float(i)*150.f;
                const float top=kinds[i]==EnemyKind::drone?380.f:505.f-sizes[i].y;
                e.box={px,top,sizes[i].x,sizes[i].y};
                e.origin={px,top}; e.base=505; e.patrol=0; e.facing=-1; e.turn=-1; e.reach=1;
                e.hp=e.maxHp=100000;
            } else e.box.x=6000;
        }
        script=[this](int frame) { input.inject(sdl::key::f,frame>80); };
        return;
    }
    if (scene=="boss") {
        prepare(0,0);
        auto& boss=game.enemies.back();
        game.player.box.x=boss.box.x-380; game.camera=boss.box.x-760;
        game.player.support=-1;
        script=[this](int frame) { input.inject(sdl::key::f,frame>10); };
        return;
    }
    if (scene.rfind("event",0)==0) {
        // eventKind or eventKind.variant, for example event4 (lightning) or event1.2 (eel).
        prepare(scene.size()>5&&scene[5]=='4'?4:0,0);
        const int kind=scene.size()>5?scene[5]-'0':0, variant=scene.size()>7?scene[7]-'0':0;
        game.player.box.x=500; game.camera=300;
        game.spawnAmbient(kind,variant);
        game.ambientTimer=99;
        return;
    }
    if (scene.rfind("arms",0)==0) {
        // Rifle lowered, steady walk or run along a long platform. armsrun adds the sprint key.
        prepare(0,0);
        game.player.box.x=1400; game.player.box.y=495-game.player.box.h; game.player.support=-1; game.camera=990;
        game.player.ready=0; game.player.readyTimer=0;
        const bool run=scene=="armsrun";
        script=[this,run](int frame) {
            input.inject(sdl::key::d,frame>=5 && frame<150);
            input.inject(sdl::key::shiftLeft,run);
        };
        return;
    }
    if (scene.rfind("hit",0)==0) {
        // hitN: one enemy of kind N (0 drone, 1 ranger, 2 turret, 3 stalker, 4 guardian) in front of the operative,
        // shot with the carbine. Frames after the first burst show the wounds, then the death animation.
        prepare(scene.size()>3&&scene[3]=='4'?1:0,0);
        const int kind=scene.size()>3?scene[3]-'0':1;
        constexpr EnemyKind kinds[]{EnemyKind::drone,EnemyKind::ranger,EnemyKind::turret,EnemyKind::stalker,EnemyKind::boss};
        constexpr Vec sizes[]{{48,30},{35,54},{48,46},{54,37},{108,97}};
        auto& player=game.player;
        player.box.x=1500; player.box.y=495-player.box.h; game.camera=1250;
        game.pointer({650,440});
        for (size_t i=0;i<game.enemies.size();++i) {
            auto& e=game.enemies[i];
            if (i!=0) { e.box.x=9000; continue; }
            e.kind=kinds[kind];
            const float top=kinds[kind]==EnemyKind::drone?410.f:495.f-sizes[kind].y;
            const float px=player.box.x+270;
            e.box={px,top,sizes[kind].x,sizes[kind].y};
            e.origin={px,top}; e.base=495; e.patrol=0; e.facing=-1; e.turn=-1;
            e.hp=e.maxHp=kind==4?400:230;
            e.damage=0; e.warning=0; e.timer=999;
        }
        followEnemy=true;
        script=[this,kind](int frame) {
            input.inject(sdl::key::f,frame>=6);
            (void)kind;
        };
        return;
    }
    if (scene=="walk") {
        prepare(0,0);
        game.pointer({900,300});
        script=[this](int frame) { input.inject(sdl::key::d,true); input.inject(sdl::key::f,frame>200); };
        return;
    }
    if (scene=="flip") {
        prepare(0,0);
        game.pointer({900,300});
        script=[this](int frame) {
            input.inject(sdl::key::d,frame<120);
            input.inject(sdl::key::space,(frame>=10 && frame<30) || (frame>=34 && frame<40));
            input.inject(sdl::key::f,frame>=36);
        };
        return;
    }
    if (scene=="slow") {
        prepare(0,0);
        game.pointer({900,420});
        script=[this](int frame) {
            input.inject(sdl::key::d,true);
            input.inject(sdl::key::q,frame>=20);
            input.inject(sdl::key::f,true);
            input.inject(sdl::key::shiftLeft,true);
        };
        return;
    }
    const int index=std::clamp(std::stoi(std::string(scene))-1,0,sectorCount-1);
    launch(index);
    game.entry=0;
    input.inject(sdl::key::d,true);
    input.inject(sdl::key::f,true);
}
} // namespace vl

namespace vl {
Rect App::followCrop(bool enabled) const {
    if (!enabled || view!=View::play) return {};
    Vec at=game.player.box.center();
    if (followEnemy && !game.enemies.empty()) at=game.enemies[0].box.center();
    // A dying enemy is a corpse by now. Follow it there.
    if (followEnemy && !game.enemies.empty() && !game.enemies[0].alive && !game.corpses.empty())
        at=game.corpses.back().body.box.center();
    at=r.screenAt(at);
    return {at.x-90,at.y-88,180,125};
}
}
