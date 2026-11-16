#include "ui/app.hpp"
#include "ui/common.hpp"
#include "ui/coop_story.hpp"
#include "render/cinematic_gait.hpp"

using namespace vl::ui;

namespace vl {
void App::coopCinematic() {
    if (mission!=1 || !loadingMusic || openingSkipped) {
        const auto& stage=*game.stage;
        r.cover(stage.art,screen,1.04f,{.52f,.48f},hex(0x91aab9));
        r.gradient(screen,hex(0x061018).alpha(.17f),hex(0x060b12).alpha(.94f));
        label({70,427},"CHAPTER "+digits(game.index+1)+" / CO-OP",teal);
        r.text({68,482},stage.name,53,white,true);
        const size_t assets=story::missionPreloads+(mission==1?story::openingArtCount:0);
        const auto status=preloadStep<assets
            ?"Loading the mission":openingSkipped?"Prologue skipped / synchronizing deployment":scheduleConfirmed?"Deployment confirmed":deployAt?"Confirming deployment":"Waiting for both operatives";
        r.text({72,519},status,18,pale);
        button({1008,516,136,42},"LEAVE",[this]{leaveCoop();});
        return;
    }
    drawOpening(true);
    button({995,528,160,35},"SKIP PROLOGUE",[this]{skipPrologue();});
}

void App::drawOpening(bool team) {
    (void)team;
    const auto& shot=story::opening.front();
    const float duration=net::openingDuration/1000.f;
    const float t=clamp(film/duration);
    const Color tint=hex(shot.tint);
    // A single image and caption persist for the whole opening. No cinematic
    // frame buffer dissolve, shot index, or multi-page progress indicators.
    r.cover(story::openingArt,screen,mix(1.05f,1.105f,cinematicEase(t)),
            mix(shot.start,shot.end,cinematicEase(t)),hex(0xe9f6f4));
    r.gradient(screen,hex(0x061018).alpha(.12f),hex(0x050b14).alpha(.82f));
    r.rect({0,0,width,27},hex(0x05090e));
    r.rect({0,568,width,32},hex(0x05090e));
    r.rect({56,393,43,2},tint);
    label({64,414},shot.location,tint);
    r.text({61,456},shot.title,40,white,true);
    label({65,485},shot.speaker,tint);
    copy({65,512},shot.line,800,18,pale);
    r.rect({65,578,1070,3},hex(0x243440));
    r.rect({65,578,1070*t,3},tint);
}
} // namespace vl
