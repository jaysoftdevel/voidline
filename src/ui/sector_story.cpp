#include "ui/app.hpp"
#include "ui/sector_story.hpp"

namespace vl {
void App::beginEpilogue() {
    film=0;
    epilogueAt=0;
    audio.pace(1);
    audio.track("");
    show(View::epilogue);
    if (coopStarted && network.hosting()) {
        epilogueAt=net::now()+guestLead;
        network.send(net::Packet::epilogue,net::encodeTiming({mission,epilogueAt,0}));
    }
}

void App::updateEpilogue(float dt) {
    const float previous=film;
    if (coopStarted) {
        const auto hostTime=network.hosting()?net::now():hostClock.estimate(net::now());
        if (!epilogueAt || hostTime<epilogueAt) return;
        film=(hostTime-epilogueAt)/1000.f;
    } else film+=dt;

    if (previous==0 && film>0) audio.track(story::track(game.index),false,film);
    const float duration=story::duration(game.index);
    if (film>=duration) {
        film=duration;
        if (!coopStarted || network.hosting()) endEpilogue();
    }
}

void App::endEpilogue() {
    if (coopStarted)
        network.send(net::Packet::epilogueDone,net::encodeTiming({mission,0,0}));
    show(View::cleared);
}

void App::skipEpilogue() {
    if (!coopStarted || network.hosting()) endEpilogue();
}
}
