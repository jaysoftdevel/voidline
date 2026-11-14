// Background events.
#include "support.hpp"
#include "render/camera_view.hpp"
#include "render/cinematic_gait.hpp"
#include "ui/sector_story.hpp"

using namespace test;
using namespace vl;

namespace {
void cinematicTimelineAndGait() {
    CHECK(story::cuts.size()==6);
    CHECK(story::cuts.front().angle==story::Angle::establish);
    CHECK(story::cuts.back().angle==story::Angle::aftermath);
    for (int i=0;i<static_cast<int>(story::cuts.size());++i) {
        CHECK(story::shotAt(story::cuts[i].at)==i);
        CHECK(story::cuts[i].dialogue>=0 && story::cuts[i].dialogue<3);
        if (i>0) CHECK(story::shotAt(story::cuts[i].at-.001f)==i-1);
    }
    for (int i=0;i<sectorCount;++i) {
        CHECK(story::duration(i)>story::cuts.back().at+5.f);
        CHECK(story::shotAt(story::duration(i))==static_cast<int>(story::cuts.size())-1);
    }
    for (float phase:{.05f,.24f,.51f,1.12f}) {
        const auto before=cinematicFoot(phase),after=cinematicFoot(phase+.05f);
        CHECK(std::abs(after.contact-before.contact+.05f*cinematicStride)<.0001f);
        CHECK(before.lift==0 && after.lift==0);
    }
    CHECK(cinematicFoot(.80f).lift>5.f);
    CHECK(std::abs(cinematicFoot(.99999f).contact-cinematicFoot(1.f).contact)<.001f);
}

void cameraProjection() {
    CameraView camera;
    const Vec points[]{{0,0},{width,0},{0,height},{width,height},{260,435},{640,280}};
    for (Vec at:points) CHECK(length(camera.sample(at)-at)<.001f);

    camera.tiltEnabled=true;
    const Vec directions[]{{-1,-1},{1,-1},{-1,1},{1,1},{0,-1},{1,0},{.6f,-.4f},{}};
    for (Vec tilt:directions) {
        camera.tilt=tilt;
        camera.focus={240,450};
        for (float zoom:{1.f,.915f}) {
            camera.zoom=zoom;
            for (Vec screen:points) {
                const Vec picked=camera.sample(screen);
                CHECK((Rect{0,0,width,height}.contains(picked)));
                CHECK(length(camera.display(picked)-screen)<.001f);
            }
        }
    }
}

void cameraSweep() {
    constexpr Vec top{width*.5f,0},bottom{width*.5f,height};
    CHECK(length(CameraTilt::target(top)-CameraTilt::target({top.x,-32}))<.001f);
    CHECK(length(CameraTilt::target(bottom)-CameraTilt::target({bottom.x,height+32}))<.001f);

    CameraTilt tilt;
    tilt.advance(CameraTilt::target(top),2);
    CameraView camera;
    camera.tiltEnabled=true;
    camera.tilt=tilt.value();
    auto span=[&] { return camera.display({1100,300}).x-camera.display({100,300}).x; };
    const float widthAtTop=span();
    float y=camera.display({840,490}).y;
    for (int frame=0;frame<240;++frame) {
        const float prior=tilt.value().y;
        tilt.advance(CameraTilt::target(bottom),1.f/120);
        camera.tilt=tilt.value();
        CHECK(camera.tilt.y>=prior-.00001f && camera.tilt.y<=1.f);
        CHECK(std::abs(span()-widthAtTop)<.001f);
        const float projected=camera.display({840,490}).y;
        CHECK(projected>=y-.001f);
        y=projected;
    }
    CHECK(std::abs(tilt.value().y-1.f)<.001f);

    // The same cursor hold produces the same angle at different frame rates.
    CameraTilt shortFrames,longFrames;
    for (int frame=0;frame<24;++frame) shortFrames.advance({1,-1},1.f/120);
    for (int frame=0;frame<6;++frame) longFrames.advance({1,-1},1.f/30);
    CHECK(length(shortFrames.value()-longFrames.value())<.00001f);
}

void ambientEvents() {
    using namespace vl;
    Silent audio;
    for (int sector = 0; sector < sectorCount; ++sector) {
        Game game(audio);
        game.start(sector, startSave());
        game.entry = 0;
        bool seen = false;
        for (int i = 0; i < 120 * 40; ++i) {
            game.tick();
            seen = seen || !game.ambient.empty();
            CHECK(game.ambient.size() <= 6);
        }
        CHECK(seen);
    }
}
}

void runWorldTests() {
    cameraProjection();
    cameraSweep();
    cinematicTimelineAndGait();
    ambientEvents();
}
