#include "render/figures/humanoid.hpp"

namespace vl {
// Builds the skeleton of a figure: pelvis, spine, head and shoulders. Pose parameters such as lean, crouch,
// breathing, landing compression and the collapse of a defeated soldier all feed into the joints.
Renderer::Rig Renderer::buildRig(const Humanoid& h) const {
    Rig g;
    g.tf=h.turn; g.fa=std::abs(g.tf); g.dir=h.stride;
    g.counter=std::sin(h.cycle*2*pi)*h.weight;
    const float idle=1-h.weight;
    g.breathe=std::sin(h.time*2.3f)*(.25f+idle*.30f+h.run*.30f);
    g.squash=h.landed*2.4f;
    const float bend=std::min(8.f,h.compression);
    const float crouch=h.crouch+h.fall*.65f+h.buckle;
    const float upper=std::min(crouch,1.35f);
    const float lean=h.lean+h.fallDirection*h.fall*.5f;

    g.hip={h.ground.x+lean*(9+upper*5)-h.flinch*2.5f*g.tf,h.ground.y-33.2f+h.run*2.2f+h.pelvis+bend+crouch*17.f};
    for (const Vec& heel:h.heels) {
        const float x=std::min(31.f,std::abs(g.hip.x-heel.x));
        g.hip.y=std::max(g.hip.y,heel.y-4.5f-std::sqrt(29.9f*29.9f-x*x));
    }
    // The neck follows the pelvis, leans with speed and sinks when the body compresses on landing.
    const float tilt=lean*(29-4*upper);
    g.neck=g.hip+Vec{tilt+g.tf*(2.4f+upper*5.8f)-h.flinch*3.f*g.tf+g.counter*.8f*g.tf,
                     -15.8f+upper*5.5f+h.tuck*.65f-g.breathe*.5f+g.squash+h.flinch*.8f};
    g.spine=std::max(8.f,length(g.neck-g.hip));
    g.up=normal(g.neck-g.hip);
    g.fwd={-g.up.y,g.up.x};
    g.waist=mix(g.hip,g.neck,.5f)+g.fwd*(g.tf*(1.1f+upper*1.8f));

    g.look=clamp(h.aim.y,-.7f,.7f);
    // The head looks toward the aim and lags behind a collapsing body.
    g.head=g.neck+Vec{g.tf*(2.2f+upper*1.8f)-h.flinch*1.6f*g.tf+h.fallDirection*h.fall*2.5f-h.headSnap*3.4f*g.tf,
                      -5.8f-h.pelvis*.4f-g.breathe*.25f+g.look*1.1f*(1-h.fall)+g.squash*.4f+h.fall*2.f-h.headSnap*1.8f};
    g.shoulder=g.neck+Vec{-1.f*g.tf-g.counter*(2.2f+h.run)*g.dir,2.6f+g.squash*.2f};
    g.farShoulder=g.shoulder+Vec{-2.4f*g.tf,.4f};
    return g;
}

// Where a point lands after the figure's flip rotation, without touching the drawing state.
Vec Renderer::spun(const Humanoid& h,Vec point) const {
    if (h.spin==0) return point;
    const Vec pivot{h.ground.x,h.ground.y-26+h.crouch*8};
    const Vec d=point-pivot;
    const float c=std::cos(h.spin),s=std::sin(h.spin);
    return pivot+Vec{d.x*c-d.y*s,d.x*s+d.y*c};
}
} // namespace vl
