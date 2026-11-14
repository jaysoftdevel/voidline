#include "support.hpp"
#include "render/cinematic_gait.hpp"

void runCinematicTests() {
    using namespace vl;
    CHECK(cinematicCruise(-1.f,0.f,4.f,.7f)==0.f);
    CHECK(cinematicCruise(5.f,0.f,4.f,.7f)==1.f);
    CHECK(std::abs(cinematicCruise(2.f,0.f,4.f,.7f)-.5f)<1e-5f);
    CHECK(cinematicWalk(0.f,0.f,4.f,.7f)==0.f);
    CHECK(cinematicWalk(4.f,0.f,4.f,.7f)==0.f);
    CHECK(cinematicWalk(2.f,0.f,4.f,.7f)==1.f);
    CHECK(cinematicEase(-1.f)==0.f);
    CHECK(cinematicEase(2.f)==1.f);
    float previous=0;
    for (int i=1;i<=400;++i) {
        const float u=i*.01f;
        const float next=cinematicCruise(u,0.f,4.f,.7f);
        CHECK(next>=previous && next<=1.f);
        previous=next;
    }
    // Integrated velocity should agree with the absolute distance curve.
    for (int i=10;i<390;++i) {
        const float time=i*.01f;
        const float derivative=(cinematicCruise(time+.001f,0,4,.7f)-cinematicCruise(time-.001f,0,4,.7f))/.002f;
        CHECK(std::abs(derivative-cinematicWalk(time,0,4,.7f)/(4.f-.7f))<.005f);
    }
    const auto stance=cinematicFoot(.60f);
    CHECK(std::abs(stance.lift)<1e-4f);
    CHECK(std::abs(cinematicFoot(1.f).contact-cinematicFoot(0.f).contact)<.001f);
}
