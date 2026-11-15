#pragma once
#include "core/math.hpp"
#include <array>
#include <cstddef>
#include <algorithm>

namespace vl {
// Rig editing coordinates: logical pixels from the ground contact point, looking right.
// These are authoring offsets to the original procedural animation, not world-space positions.
inline constexpr size_t skeletonJointCount=14, skeletonPoseCount=3;
inline constexpr size_t actionPoseCount=18;
enum class SkeletonJoint : size_t {
    pelvis,waist,neck,head,nearShoulder,farShoulder,nearElbow,farElbow,
    nearHand,farHand,nearKnee,farKnee,nearAnkle,farAnkle
};
enum class SkeletonPose : size_t { base,aim,run };
inline constexpr std::array<const char*,skeletonJointCount> skeletonJointNames{{
    "pelvis","waist","neck","head","nearShoulder","farShoulder","nearElbow",
    "farElbow","nearHand","farHand","nearKnee","farKnee","nearAnkle","farAnkle"
}};
inline constexpr std::array<const char*,skeletonPoseCount> skeletonPoseNames{{"base","aim","run"}};
inline constexpr std::array<const char*,actionPoseCount> actionPoseNames{{
    "idle","walk","sprint","crouch","jump","fall","land","fire","reload",
    "hit","death","patrol","chase","alert","recoil","stagger","stairs","interact"
}};
enum class ActionPose : int { idle,walk,sprint,crouch,jump,fall,land,fire,reload,hit,death,
                              patrol,chase,alert,recoil,stagger,stairs,interact,none=-1 };
inline constexpr std::array<Vec,skeletonJointCount> skeletonReference{{
    {0,-33},{0,-41},{2,-49},{4,-59},{6,-46},{-5,-46},{14,-35},
    {-13,-35},{18,-28},{-16,-29},{8,-19},{-8,-19},{10,-5},{-10,-5}
}};
// Bind-pose Z positions, in logical model units: farther limbs recede behind the torso.
inline constexpr std::array<float,skeletonJointCount> skeletonReferenceDepth{{
    0.f,0.f,0.f,0.f,7.f,-7.f,8.f,-8.f,9.f,-9.f,6.f,-6.f,7.f,-7.f
}};
inline constexpr std::array<std::array<int,2>,13> skeletonBones{{
    {{0,1}},{{1,2}},{{2,3}},{{2,4}},{{2,5}},{{4,6}},{{6,8}},
    {{5,7}},{{7,9}},{{0,10}},{{10,12}},{{0,11}},{{11,13}}
}};
struct SkeletonProfile {
    // Per-pose offsets from the editor's authored reference skeleton.
    std::array<std::array<Vec,skeletonJointCount>,skeletonPoseCount> offsets{};
    // Depth-axis vector offsets. Screen-space game animation still uses XY;
    // the side/top/3D authoring views preserve authored depth for future rigs.
    std::array<std::array<float,skeletonJointCount>,skeletonPoseCount> depths{};
    // Supplemental per-action poses are cached at startup from the game asset pack.
    // They are not stored in the legacy VLRM/VLEM fixed records; older archives
    // therefore keep precisely the same appearance if no action file exists.
    std::array<std::array<Vec,skeletonJointCount>,actionPoseCount> actionOffsets{};
    // All scales default to 1.0 so old characters preserve their exact appearance.
    float stride=1.f,armSwing=1.f,aimRaise=1.f;
    float bodyHeight=1.f,bodyWidth=1.f,headScale=1.f;
    float armLength=1.f,legLength=1.f,weaponSeat=0.f;
    // Motion timing, braking and procedural reaction blend factors.
    float walkCycle=1.f,acceleration=1.f,braking=1.f,recoil=1.f,idleBreath=1.f;
};
inline Vec skeletonOffset(const SkeletonProfile& s,SkeletonJoint joint,float aiming,float running,
                          ActionPose action=ActionPose::none,float actionWeight=0.f){
    const auto j=static_cast<size_t>(joint);
    const Vec b=s.offsets[0][j],a=s.offsets[1][j],r=s.offsets[2][j];
    Vec result=b+a*std::clamp(aiming,0.f,1.f)+r*std::clamp(running,0.f,1.f);
    const int aidx=static_cast<int>(action);
    if(aidx>=0 && aidx<static_cast<int>(actionPoseCount))
        result+=s.actionOffsets[static_cast<size_t>(aidx)][j]*std::clamp(actionWeight,0.f,1.f);
    return result;
}
// Canonical editable bind pose, with dimensions applied consistently in the editor
// and used as the reference for game-side procedural rig attachment points.
inline Vec modelJoint(const SkeletonProfile& profile,SkeletonJoint joint,float aiming=0,float running=0) {
    const auto id=static_cast<size_t>(joint);
    Vec p=skeletonReference[id];
    p.x*=profile.bodyWidth;
    p.y*=profile.bodyHeight;
    if (id>=static_cast<size_t>(SkeletonJoint::nearElbow) &&
        id<=static_cast<size_t>(SkeletonJoint::farHand)) {
        const Vec anchor=skeletonReference[static_cast<size_t>(SkeletonJoint::neck)];
        p.y=(anchor.y+(skeletonReference[id].y-anchor.y)*profile.armLength)*profile.bodyHeight;
    } else if (id>=static_cast<size_t>(SkeletonJoint::nearKnee)) {
        const Vec hip=skeletonReference[static_cast<size_t>(SkeletonJoint::pelvis)];
        p.y=(hip.y+(skeletonReference[id].y-hip.y)*profile.legLength)*profile.bodyHeight;
    } else if (joint==SkeletonJoint::head) {
        p.y=(skeletonReference[static_cast<size_t>(SkeletonJoint::neck)].y +
             (skeletonReference[id].y-skeletonReference[static_cast<size_t>(SkeletonJoint::neck)].y)*profile.headScale)*profile.bodyHeight;
    }
    return p+skeletonOffset(profile,joint,aiming,running);
}
} // namespace vl
