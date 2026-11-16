#include "render/figures/humanoid.hpp"
#include "content/skeleton.hpp"

namespace vl {

// Both legs. The far leg is drawn first and darker. Each leg is a boot, a shin with a greave, a thigh with a
// plate, and a knee pad over the joint. The knee always bends forward, like a human knee.
void Renderer::drawLegs(const Humanoid& h,const Rig& g) {
    using Look=Humanoid::Look;
    const Color ink=h.ink;
    const int dir=g.dir;
    auto leg=[&](int i,bool far) {
        Vec contact=h.heels[i];
        if(h.model){
            const auto& profile=*h.model;
            contact.x=h.ground.x+(contact.x-h.ground.x)*(1.f+(profile.stride-1.f)*h.weight);
        }
        const Vec toeRay=ray(h.footAngle[i]);
        const Vec forward{toeRay.x*dir,-toeRay.y};
        const float lift=h.grounded?std::max(0.f,std::max(contact.y,(contact+forward*8).y)-h.ground.y):0.f;
        Vec heel=contact-Vec{0,2.4f+lift};
        Vec toe=contact+forward*8-Vec{0,2.4f+lift};
        const float rotation=g.pelvisRoll*(i==0?-.65f:.65f);
        const Vec socket=g.hip+Vec{i==0?-2.f:2.f,(i==0?-1.f:.5f)+rotation};
        const float scale=h.model?h.model->legLength*h.model->bodyHeight:1.f;
        const float thigh=15.6f*scale,shin=14.6f*scale;
        Vec ankleShift{},kneeShift{};
        if(h.model){
            const auto& profile=*h.model;
            const auto shift=[&](SkeletonJoint joint){
                const Vec d=skeletonOffset(profile,joint,h.shoulder*profile.aimRaise,std::max(h.run,h.weight*.84f),h.action,h.actionWeight,h.actionPhase);
                return Vec{d.x*(h.turn<0?-1.f:1.f),d.y};
            };
            const bool near=i==1;
            ankleShift=shift(near?SkeletonJoint::nearAnkle:SkeletonJoint::farAnkle);
            kneeShift=shift(near?SkeletonJoint::nearKnee:SkeletonJoint::farKnee);
        }
        // Ankle, heel and boot always travel together. A springy jump can
        // leave a simulated heel behind the airborne pelvis; pulling only
        // the knee within reach would produce a visibly stretched shin.
        const Vec desired=heel-Vec{0,2.1f}+ankleShift;
        const Vec ankle=reachableAnkle(socket,desired,thigh,shin);
        const Vec correction=ankle-desired+ankleShift;
        heel+=correction;
        toe+=correction;
        Vec knee=solveKnee(socket,ankle,thigh,shin,dir);
        // Authoring a knee pose changes its bend subtly, but cannot lengthen
        // the bones substantially. Use the perpendicular component only.
        const Vec ikAxis=normal(ankle-socket);
        const Vec ikNormal{ikAxis.y,-ikAxis.x};
        knee+=ikNormal*clamp(dot(kneeShift,ikNormal),-.5f,.5f);
        const Color cloth=far?h.shadow:h.suit;
        const Color plate=far?h.shadow.energy(1.4f):h.shell;
        const Color boot=far?h.leather.energy(.6f):h.leather;
        // Boot: outline, upper, rubber sole with tread, toe cap and heel block.
        line(heel-Vec{2.7f*dir,0},toe+Vec{1.4f*dir,0},far?5.2f:6.2f,ink);
        line(heel-Vec{2.1f*dir,.5f},toe,far?3.4f:4.4f,boot);
        line(heel-Vec{2.4f*dir,2.f},toe+Vec{.9f*dir,2.f},1.5f,hex(far?0x14171d:0x262b34));
        if (!far) {
            for (int t=0;t<3;++t) {
                const Vec p=mix(heel,toe,.2f+t*.3f)+Vec{0,2.4f};
                line(p,p+Vec{0,.9f},.8f,ink);
            }
            line(heel-Vec{1.6f*dir,-.6f},toe-Vec{0,.9f},.8f,plate.alpha(.7f));
        }
        ellipse(toe-Vec{.8f*dir,.1f},{far?2.3f:2.8f,far?2.2f:2.6f},plate.energy(far?.8f:1.f));
        polygon(heel-Vec{2.8f*dir,1.f},heel-Vec{.4f*dir,1.f},heel-Vec{.4f*dir,-2.6f},heel-Vec{2.8f*dir,-2.6f},plate.energy(.75f));
        // Thigh, shin, greave and the plate over the thigh.
        armor(socket,knee,far?3.3f:4.5f,far?2.4f:3.3f,cloth,h.edge,!far);
        armor(knee,ankle,far?2.6f:3.4f,far?1.9f:2.5f,cloth.energy(.92f),h.edge,!far);
        armor(mix(knee,ankle,.2f),mix(knee,ankle,.86f),far?1.8f:2.1f,far?1.5f:1.7f,plate,h.edge,!far);
        ellipse(ankle,{far?2.6f:3.3f,2.2f},cloth.energy(.8f));
        if (!far) {
            armor(mix(socket,knee,.28f),mix(socket,knee,.66f),2.5f,2.4f,h.look==Look::soldier?plate:h.trim.energy(.72f),h.edge,true);
            const Vec strap=mix(socket,knee,.5f);
            line(strap-Vec{3.6f,0},strap+Vec{3.6f,0},1.1f,ink.alpha(.8f));
            ellipse(mix(socket,knee,.46f),{.8f,.8f},h.edge);
            // Laces across the instep.
            for (int t=0;t<2;++t) {
                const Vec p=mix(ankle,toe,.42f+t*.22f)-Vec{0,1.2f};
                line(p-Vec{1.1f,0},p+Vec{1.1f,0},.8f,h.trim.alpha(.9f));
            }
        }
        if (h.cinematic && !far) {
            // Segmented shin guard and flexible inner joint, separate from boot.
            const Vec brace0=mix(knee,ankle,.28f),brace1=mix(knee,ankle,.72f);
            const Vec axis=normal(brace1-brace0),normalAxis{axis.y,-axis.x};
            line(brace0+normalAxis*1.05f,brace1+normalAxis*.75f,.84f,h.edge.energy(1.13f));
            for (int k=0;k<3;++k) {
                const Vec seam=mix(brace0,brace1,.19f+.30f*k);
                line(seam-normalAxis*1.4f,seam+normalAxis*1.8f,.62f,ink.alpha(.82f));
            }
            line(mix(socket,knee,.22f),mix(socket,knee,.77f),.75f,h.edge.alpha(.77f));
            ellipse(mix(socket,knee,.37f),{.72f,.72f},h.trim.energy(1.15f));
        }
        if (!far && h.look==Look::mercer) {
            // Extra shin and heavy calf shield make Jax visibly stockier at gameplay scale.
            armor(mix(knee,ankle,.15f),mix(knee,ankle,.68f),4.6f,4.1f,h.trim.energy(.72f),h.edge,true);
            line(mix(knee,ankle,.35f)-Vec{3.2f,0},mix(knee,ankle,.35f)+Vec{3.2f,0},1.2f,ink);
        } else if (!far && h.look==Look::cole) {
            line(mix(knee,ankle,.22f)+Vec{2.f,0},mix(knee,ankle,.81f)+Vec{2.f,0},1.2f,h.trim.energy(1.9f));
            triangle(toe+Vec{-2.f,-2.f},toe+Vec{4.f*dir,-2.f},toe+Vec{-2.f,-5.f},h.shell);
        } else if (!far && h.look==Look::nyx) {
            armor(mix(knee,ankle,.2f),mix(knee,ankle,.82f),2.2f,2.1f,hex(0xd1cbd7),h.edge,true);
            ellipse(mix(knee,ankle,.55f),{1.f,2.f},h.trim.energy(2.2f));
        }
        // Knee pad.
        ellipse(knee,{far?2.7f:3.5f,far?2.9f:3.8f},ink);
        ellipse(knee,{far?2.0f:2.7f,far?2.2f:3.0f},plate.energy(far?.9f:1.1f));
        if (!far) {
            line(knee+Vec{-2.2f,-1.2f},knee+Vec{2.f,-1.6f},.9f,h.edge.alpha(.85f));
            line(knee+Vec{-1.9f,.9f},knee+Vec{2.1f,1.1f},1.2f,h.trim.energy(1.15f));
        }
    };
    // In a close shot the far leg keeps its depth and shading as the feet cross.
    const int rear=h.cinematic?0:(h.heels[0].x*dir<h.heels[1].x*dir?0:1);
    leg(rear,true); leg(1-rear,false);
}
} // namespace vl
