#include "render/figures/humanoid.hpp"
#include "content/skeleton.hpp"

namespace vl {
namespace {
constexpr float upperLength=11.6f,foreLength=10.6f;

// Unit vector of a limb segment. Angle zero hangs straight down, positive angles swing toward the facing side
// and past a right angle the segment points up.
Vec bone(float angle,int facing) { return {std::sin(angle)*facing,std::cos(angle)}; }

float bendBetween(Vec shoulder,Vec elbow,Vec wrist) {
    return std::acos(clamp(dot(normal(elbow-shoulder),normal(wrist-elbow)),-1.f,1.f));
}
}

// A free arm follows the stride the way a walking human does: the shoulder swings against the same-side leg,
// about 20 degrees when walking and 45 degrees when running, and the elbow flexes more as the arm swings forward.
// The forearm only ever folds toward the front of the body.
Renderer::ArmPose Renderer::freeArm(const Humanoid& h,const Rig& g,Vec s,bool near) const {
    const int facing=g.tf<0?-1:1;
    const float armScale=h.model?h.model->armLength:1.f;
    const float authoredSwing=h.model?h.model->armSwing:1.f;
    // g.counter already indicates which anatomical foot is in front, in
    // travel-local coordinates. Multiplying by facing again would reverse
    // the arm/leg relationship whenever the character turns around.
    const float phase=(near?-1.f:1.f)*g.counter*authoredSwing;
    const float sway=std::sin(h.time*1.3f+(near?0.f:1.7f))*.07f*(1-h.weight);
    const float swing=clamp(phase+sway,-1.f,1.f);
    // Humans swing the arm further forward than back: about 40 degrees forward and 30 back at a run.
    float upper=(swing>0?swing*mix(.30f,.70f,h.run):swing*mix(.22f,.52f,h.run))+.04f+h.crouch*.12f;
    float flex=mix(.22f,1.00f,h.run)+mix(.20f,.46f,h.run)*(swing*.5f+.5f)+h.crouch*.2f;
    if (h.fall>0) {
        // A collapsing body throws its arms out and lets them go limp.
        const float flail=std::sin(h.time*11.f+(near?0.f:2.2f))*.45f;
        upper=mix(upper,-.95f+flail,h.fall);
        flex=mix(flex,.55f,h.fall);
    }
    // The shoulder blade glides with the arm instead of staying pinned.
    s=s+Vec{facing*swing*1.1f,-std::abs(swing)*.3f};
    ArmPose arm;
    arm.shoulder=s;
    arm.elbow=s+bone(upper,facing)*(upperLength*armScale);
    arm.wrist=arm.elbow+bone(upper+flex,facing)*(foreLength*armScale);
    arm.hand=arm.wrist+bone(upper+flex+.12f,facing)*3.f;
    arm.flexion=flex;
    if(h.model){
        const auto& profile=*h.model;
        const auto shift=[&](SkeletonJoint joint){
            const Vec d=skeletonOffset(profile,joint,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
            return Vec{d.x*facing,d.y};
        };
        arm.elbow+=shift(near?SkeletonJoint::nearElbow:SkeletonJoint::farElbow);
        arm.wrist+=shift(near?SkeletonJoint::nearHand:SkeletonJoint::farHand);
        arm.hand+=shift(near?SkeletonJoint::nearHand:SkeletonJoint::farHand);
    }
    return arm;
}

// An arm reaching for a point, such as a rifle grip. The point is pulled back inside the arm's reach so the
// arm never stretches, and the elbow hangs below and behind the line from shoulder to hand.
Renderer::ArmPose Renderer::gripArm(const Humanoid& h,Vec s,Vec hand,bool near) const {
    const int facing=h.turn<0?-1:1;
    const float armScale=h.model?h.model->armLength:1.f;
    const float upper=upperLength*armScale,fore=foreLength*armScale;
    const Vec delta=hand-s;
    const float distance=length(delta);
    const float reach=clamp(distance,std::abs(upper-fore)+1.f,(upper+fore)*.985f);
    const Vec target=distance>.001f?s+delta*(reach/distance):s+Vec{0,reach};
    const Vec a=solveKnee(s,target,upper,fore,1),b=solveKnee(s,target,upper,fore,-1);
    auto score=[&](Vec e) { return e.y-facing*e.x*.8f; };
    ArmPose arm;
    arm.shoulder=s;
    if (h.ladder && !h.ladderAiming) {
        // The hands reach forward to the rungs while the elbows bend toward
        // the ladder's center line, between the chest and the rung assembly.
        // Keep the two-bone IK lengths intact; simply choose the inner of
        // the two valid elbow solutions rather than the usual weapon bend.
        arm.elbow=std::abs(a.x-h.ground.x)<std::abs(b.x-h.ground.x)?a:b;
    } else {
        arm.elbow=score(a)>score(b)?a:b;
    }
    arm.hand=target;
    arm.wrist=arm.elbow+normal(target-arm.elbow)*std::max(4.f,length(target-arm.elbow)-2.8f);
    arm.flexion=bendBetween(s,arm.elbow,arm.wrist);
    if(h.model){
        const auto& profile=*h.model;
        const auto shift=[&](SkeletonJoint joint){
            const Vec d=skeletonOffset(profile,joint,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
            return Vec{d.x*facing,d.y};
        };
        arm.elbow+=shift(near?SkeletonJoint::nearElbow:SkeletonJoint::farElbow);
        arm.wrist+=shift(near?SkeletonJoint::nearHand:SkeletonJoint::farHand)*.6f;
    }
    return arm;
}

// Joint-wise blend that keeps both bones at their real length.
Renderer::ArmPose Renderer::blendArms(const ArmPose& a,const ArmPose& b,float t,float armScale) {
    ArmPose arm;
    arm.shoulder=a.shoulder;
    arm.elbow=a.shoulder+normal(mix(a.elbow,b.elbow,t)-a.shoulder)*(upperLength*armScale);
    arm.wrist=arm.elbow+normal(mix(a.wrist,b.wrist,t)-arm.elbow)*(foreLength*armScale);
    arm.hand=mix(a.hand,b.hand,t);
    arm.flexion=mix(a.flexion,b.flexion,t);
    return arm;
}

void Renderer::drawArms(const Humanoid& h,const Rig& g) {
    Vec nearHand=h.nearHand,farHand=h.farHand;
    const float armScale=h.model?h.model->armLength:1.f;
    if (h.gripFromChest) {
        const Vec mount=g.shoulder+Vec{-g.tf*3.f*h.shoulder,5.f-4.f*h.shoulder};
        nearHand=mount+h.aim*(h.gripNear-h.recoil*3.f);
        farHand=mount+h.aim*(h.gripFar-h.recoil*3.f);
    }
    if(h.model){
        const auto& profile=*h.model;
        const float faced=h.turn<0?-1.f:1.f;
        const Vec n=skeletonOffset(profile,SkeletonJoint::nearHand,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
        const Vec f=skeletonOffset(profile,SkeletonJoint::farHand,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
        nearHand+=Vec{n.x*faced,n.y};farHand+=Vec{f.x*faced,f.y};
    }
    // A collapsed figure drops the weapon and both arms go free.
    const bool dropped=h.fall>.12f;
    const float relax=h.farFree||dropped?1.f:h.relax;
    ArmPose far=gripArm(h,g.farShoulder,farHand,false);
    if (relax>0) far=blendArms(far,freeArm(h,g,g.farShoulder,false),relax,armScale);
    const float nearRelax=dropped?1.f:h.nearRelax;
    ArmPose near=gripArm(h,g.shoulder,nearHand,true);
    if (nearRelax>0) near=blendArms(near,freeArm(h,g,g.shoulder,true),nearRelax,armScale);
    drawArm(h,far,false,relax<.5f);
    if (h.weapon && !dropped) h.weapon(h.cinematic?near.hand:nearHand,h.cinematic?far.hand:farHand);
    drawArm(h,near,true,nearRelax<.5f);
}

// One arm: upper arm with a biceps, forearm with a bracer, an elbow pad on the outside of the bend, creases on
// the inside, a pauldron over the shoulder and the hand.
void Renderer::drawArm(const Humanoid& h,const ArmPose& a,bool near,bool gripping) {
    using Look=Humanoid::Look;
    const Color ink=h.ink;
    const Color sleeve=near?h.suit:h.shadow;
    const Color plate=near?h.shell:h.shadow.energy(1.3f);
    const Vec s=a.shoulder,e=a.elbow,w=a.wrist;
    const Vec inside=normal(normal(s-e)+normal(w-e));
    const Vec across{inside.y,-inside.x};
    const float fold=clamp(a.flexion/1.5f);
    armor(s,e,near?4.1f:3.2f,near?2.8f:2.3f,sleeve,h.edge,near);
    if (near && fold>.15f) {
        ellipse(mix(s,e,.5f)+inside*(1.1f*fold),{2.6f,2.2f+fold},sleeve.energy(1.18f));
        line(mix(s,e,.34f)+inside*(1.4f*fold)-across*1.8f,mix(s,e,.34f)+inside*(1.4f*fold)+across*1.8f,.7f,ink.alpha(.45f));
    }
    armor(e,w,near?2.9f:2.4f,near?2.1f:1.9f,sleeve.energy(.95f),h.edge,near);
    armor(mix(e,w,.34f),mix(e,w,.9f),near?3.3f:2.7f,near?2.8f:2.3f,plate,h.edge,near);
    // Elbow pad on the outside of the bend, with creases opposite.
    const Vec pad=e-inside*(1.3f*fold);
    ellipse(pad,{near?2.9f:2.0f,near?2.8f:1.9f},ink);
    ellipse(pad,{near?2.2f:1.4f,near?2.1f:1.3f},plate.energy(near?1.15f:.95f));
    if (near && fold>.45f) {
        for (int k=0;k<2;++k) {
            const Vec c=e+inside*(1.5f+k*.9f);
            line(c-across*1.3f,c+across*1.3f,.7f,ink.alpha(.55f));
        }
    }
    if (near) {
        const Vec light=mix(e,w,.64f);
        ellipse(light,{1.3f,.9f},h.trim.energy(2.4f));
        const Vec band=mix(e,w,.42f);
        line(band-across*1.9f,band+across*1.9f,.8f,ink.alpha(.6f));
    }
    if (h.cinematic && near) {
        // Flex-fabric elbow pleats, layered bracer and three tiny status studs.
        for (int i=0;i<3;++i) {
            const Vec spot=mix(e,w,.45f+i*.135f);
            line(spot-across*1.85f,spot+across*1.85f,.63f,h.edge.energy(i==0?1.4f:.85f));
        }
        const Vec badge=mix(e,w,.69f)-across*1.6f;
        ellipse(badge,{.73f,.73f},h.trim.energy(1.7f));
        line(mix(s,e,.16f),mix(s,e,.69f),.68f,h.edge.alpha(.7f));
    }
    // Pauldron over the joint.
    const Color cap=near?(h.look==Look::voss?h.trim.energy(.95f):h.shell.energy(1.12f)):h.shadow.energy(1.5f);
    const Vec armAxis=normal(e-s),armSide{armAxis.y,-armAxis.x};
    ellipse(s,{near?4.8f:3.7f,near?4.3f:3.3f},ink);
    ellipse(s,{near?4.0f:2.9f,near?3.5f:2.5f},cap);
    if (near && h.look==Look::mercer) {
        // Angular shield-pauldron, a full extra plate outside the shoulder.
        polygon(s-armSide*6.f-armAxis*2.8f,s+armSide*6.f-armAxis*2.8f,
                s+armSide*6.2f+armAxis*5.9f,s-armSide*6.f+armAxis*5.9f,h.trim.energy(.82f));
        line(s-armSide*4.4f,s+armSide*4.4f,1.5f,hex(0xf2c495).alpha(.9f));
    } else if (near && h.look==Look::cole) {
        triangle(s-armSide*4.6f-armAxis*3.f,s+armSide*4.4f-armAxis*3.1f,
                 s+armAxis*7.1f,h.shell.energy(1.4f));
        ellipse(s+armAxis*2.f,{1.2f,1.3f},h.trim.energy(2.3f));
    } else if (near && h.look==Look::nyx) {
        polygon(s-armSide*4.6f-armAxis*1.8f,s+armSide*4.6f-armAxis*1.8f,
                s+armSide*3.3f+armAxis*5.f,s-armSide*3.3f+armAxis*5.f,hex(0xdcd7df));
        line(s+armSide*3.f-armAxis*.5f,s+armAxis*4.8f,1.3f,h.trim.energy(2.f));
    }
    if (near) {
        line(s-armSide*3.2f+armAxis*.5f,s+armSide*3.2f+armAxis*.5f,.9f,h.edge.alpha(.85f));
        line(s-armSide*3.6f+armAxis*2.6f,s+armSide*3.6f+armAxis*2.6f,1.3f,h.look==Look::voss?ink.alpha(.75f):h.trim.energy(1.1f));
    }
    drawHand(h,a,near,gripping);
}

// A closed fist around a grip, or a relaxed open hand with slightly curled fingers and a thumb.
void Renderer::drawHand(const Humanoid& h,const ArmPose& a,bool near,bool gripping) {
    const Color ink=h.ink;
    const Vec along=normal(a.hand-a.wrist);
    const int facing=h.turn<0?-1:1;
    Vec curl{-along.y,along.x};
    if (curl.x*facing<0) curl=curl*-1.f;
    if (gripping) {
        const Vec c=a.hand;
        ellipse(c,{near?3.2f:2.7f,near?3.f:2.5f},ink);
        ellipse(c,{near?2.5f:2.1f,near?2.3f:1.9f},h.glove);
        for (int k=0;k<3;++k) {
            const Vec k0=c+along*.9f+curl*(.2f+k*.8f)-Vec{0,1.3f};
            line(k0,k0+along*1.2f,.6f,ink.alpha(.75f));
        }
        line(c-along*.6f-curl*1.1f,c+along*1.2f-curl*.6f,1.3f,h.glove.energy(1.25f));
        return;
    }
    const Vec palm=a.wrist+along*2.2f;
    ellipse(palm,{near?2.9f:2.5f,near?2.7f:2.3f},ink);
    ellipse(palm,{near?2.2f:1.9f,near?2.0f:1.7f},h.glove);
    for (int k=0;k<4;++k) {
        const float c1=.30f+k*.10f,c2=.55f+k*.12f;
        const Vec base=palm+along*1.7f+Vec{along.y,-along.x}*((k-1.5f)*.95f)*float(facing);
        const Vec mid=base+(along*std::cos(c1)+curl*std::sin(c1))*(1.9f-k*.1f);
        const Vec tip=mid+(along*std::cos(c1+c2)+curl*std::sin(c1+c2))*(1.6f-k*.1f);
        line(base,mid,1.7f,ink); line(mid,tip,1.5f,ink);
        line(base,mid,1.0f,h.glove); line(mid,tip,.9f,h.glove);
    }
    const Vec thumbBase=palm-curl*1.4f+along*.4f;
    const Vec thumbTip=thumbBase+(along*.7f-curl*.5f)*2.6f;
    line(thumbBase,thumbTip,1.8f,ink); line(thumbBase,thumbTip,1.1f,h.glove);
}
} // namespace vl
