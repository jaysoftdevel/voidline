#include "render/figures/humanoid.hpp"
#include "content/skeleton.hpp"
#include "game/running_cycle.hpp"

namespace vl {
// Builds the skeleton of a figure: pelvis, spine, head and shoulders. Pose parameters such as lean, crouch,
// breathing, landing compression and the collapse of a defeated soldier all feed into the joints.
Renderer::Rig Renderer::buildRig(const Humanoid& h) const {
    Rig g;
    g.tf=h.turn; g.fa=std::abs(g.tf); g.dir=h.stride;
    // Determine the active lead foot from the interpolated heel positions.
    // This keeps shoulder rotation tied to each actual step rather than an
    // unrelated sine phase, including changes of facing or travel direction.
    const float travelAxis=std::abs(h.velocity.x)>12.f?
        (h.velocity.x<0.f?-1.f:1.f):float(h.facing<0?-1:1);
    const float lead=running::footLead(h.heels[1].x,h.heels[0].x,travelAxis);
    const float locomotion=clamp(h.weight*(1.f-h.tuck)*(1.f-h.fall)*(h.ladder?0.f:1.f));
    g.counter=lead*locomotion;
    // Hip rotation follows the leading leg; the upper body counter-rotates.
    // Suppress rotation during jumping, falling, climbing and collapse.
    const float gait=locomotion*mix(.30f,1.f,h.run);
    g.pelvisRoll=lead*gait;
    const float idle=1-h.weight;
    const auto* model=h.model;
    g.breathe=std::sin(h.time*2.3f)*(.25f+idle*.30f+h.run*.30f)*(model?model->idleBreath:1.f);
    g.squash=h.landed*2.4f;
    const float bend=std::min(8.f,h.compression);
    const float crouch=h.crouch+h.fall*.65f+h.buckle;
    const float upper=std::min(crouch,1.35f);
    const float lean=h.lean+h.fallDirection*h.fall*.5f;

    g.hip={h.ground.x+lean*(9+upper*5)-h.flinch*2.5f*g.tf+g.pelvisRoll*travelAxis*1.25f,
           h.ground.y-33.2f*(model?model->bodyHeight:1.f)+h.run*2.2f+h.pelvis+bend+crouch*17.f};
    // A grounded pelvis follows planted feet to avoid stretching the thigh.
    // Airborne feet are allowed to tuck up instead of dragging the pelvis down.
    if (h.grounded && !h.ladder) for (const Vec& heel:h.heels) {
        const float legReach=29.9f*(model?model->legLength*model->bodyHeight:1.f);
        const float x=std::min(legReach*.96f,std::abs(g.hip.x-heel.x));
        g.hip.y=std::max(g.hip.y,heel.y-4.5f-std::sqrt(legReach*legReach-x*x));
    }
    // The neck follows the pelvis, leans with speed and sinks when the body compresses on landing.
    const float tilt=lean*(29-4*upper);
    const float torsoScale=model?model->bodyHeight:1.f;
    const float widthScale=model?model->bodyWidth:1.f;
    g.neck=g.hip+Vec{tilt+g.tf*(2.4f+upper*5.8f)-h.flinch*3.f*g.tf
                         -g.pelvisRoll*travelAxis*(2.10f+.35f*h.run),
                     -15.8f*torsoScale+upper*5.5f+h.tuck*.65f-g.breathe*.5f+g.squash+h.flinch*.8f};
    g.spine=std::max(8.f,length(g.neck-g.hip));
    g.up=normal(g.neck-g.hip);
    g.fwd={-g.up.y,g.up.x};
    g.waist=mix(g.hip,g.neck,.5f)+g.fwd*(g.tf*(1.1f+upper*1.8f));

    g.look=clamp(h.aim.y,-.7f,.7f);
    // The head looks toward the aim and lags behind a collapsing body.
    g.head=g.neck+Vec{g.tf*(2.2f+upper*1.8f)-h.flinch*1.6f*g.tf+h.fallDirection*h.fall*2.5f
                         -h.headSnap*3.4f*g.tf+g.pelvisRoll*travelAxis*.65f,
                      -5.8f*(model?model->headScale:1.f)-h.pelvis*.4f-g.breathe*.25f+g.look*1.1f*(1-h.fall)+g.squash*.4f+h.fall*2.f-h.headSnap*1.8f};
    // The leading foot's shoulder stays BEHIND the center of the chest.
    // Its opposite shoulder moves the same distance forward. Near and far
    // shoulders share a pivot, so the torso never acquires a fixed screen-
    // facing twist from movement. Motion grows from walking to sprinting.
    const float shoulderStrength=locomotion*mix(1.8f,4.5f,clamp(h.run))
        *(1.f-.28f*clamp(h.shoulder));
    const auto swing=running::counterShoulders(lead,shoulderStrength);
    const Vec center=g.neck+Vec{-2.2f*g.tf*widthScale,2.7f+g.squash*.2f};
    const float staticOffset=1.2f*g.tf*widthScale*(1.f-locomotion);
    g.shoulder=center+Vec{staticOffset+swing.near*travelAxis,
                          -.17f*swing.near};
    g.farShoulder=center+Vec{-staticOffset+swing.far*travelAxis,
                             -.17f*swing.far+.4f};
    if (h.ladder) {
        // Square shoulders about a vertical spine when facing the ladder.
        // Even during shooting the pelvis stays centered on its axis; only
        // the upper chest and weapon arms are allowed to turn toward aim.
        const float aimTurn=h.ladderAiming?.52f:0.f;
        g.shoulder=mix(center+Vec{-7.f*widthScale,1.2f},g.shoulder,aimTurn);
        g.farShoulder=mix(center+Vec{7.f*widthScale,1.2f},g.farShoulder,aimTurn);
        g.neck.x=g.hip.x+(g.neck.x-g.hip.x)*aimTurn;
        g.head.x=g.neck.x+(g.head.x-g.neck.x)*aimTurn;
    }
    if(model){
        // Blend authored base / shouldered-aim / run joint vectors into the original
        // procedural gait. Mirroring is applied only at draw time, never in JSON.
        const auto& profile=*model;
        const auto shift=[&](SkeletonJoint j){
            const Vec d=skeletonOffset(profile,j,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
            return Vec{d.x*(h.turn<0?-1.f:1.f),d.y};
        };
        g.hip+=shift(SkeletonJoint::pelvis);
        g.waist+=shift(SkeletonJoint::waist);
        g.neck+=shift(SkeletonJoint::neck);
        g.head+=shift(SkeletonJoint::head);
        g.shoulder+=shift(SkeletonJoint::nearShoulder);
        g.farShoulder+=shift(SkeletonJoint::farShoulder);
        g.spine=std::max(8.f,length(g.neck-g.hip));
        g.up=normal(g.neck-g.hip);
        g.fwd={-g.up.y,g.up.x};
    }
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
