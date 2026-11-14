#pragma once
#include "render/renderer.hpp"
#include <functional>

namespace vl {
// A side-view human rig. The operatives and the soldier enemies share it. All positions are in screen space.
struct Renderer::Humanoid {
    enum class Look { voss, rook, vale, mercer, cole, nyx, soldier };
    Look look=Look::soldier;
    Vec ground;
    int facing=1,stride=1;
    float turn=1;
    std::array<Vec,2> heels{};
    std::array<float,2> footAngle{};
    bool grounded=true;
    float lean=0,crouch=0,run=0,weight=0,cycle=0,compression=0,pelvis=0,tuck=0;
    float time=0,flinch=0,landed=0,recoil=0,spin=0,speech=0,blink=0;
    bool cinematic=false;
    Vec velocity,aim{1,0},nearHand,farHand;
    bool farFree=false,gripFromChest=false,scarf=true;
    float gripNear=9,gripFar=19;
    // 0 when both hands hold the weapon, 1 when the far arm hangs relaxed and swings with the stride.
    float relax=0,nearRelax=0;
    // 0 standing, 1 collapsed. Defeated soldiers buckle at the knees, fall back and let their arms go limp.
    float fall=0,fallDirection=1;
    // Hit reactions by body part: the head snaps back, the knees buckle.
    float headSnap=0,buckle=0;
    Color suit,shell,trim,edge,shadow,ink,skin,hair,visor,leather,glove;
    std::function<void(Vec,Vec)> weapon;
};

// Joint positions derived from the pose parameters of one figure.
struct Renderer::Rig {
    Vec hip,waist,neck,head,shoulder,farShoulder,up,fwd;
    float spine=0,tf=1,fa=1,breathe=0,squash=0,counter=0,look=0;
    int dir=1;
    // A point along the spine (u) and across the body (v, toward the facing side).
    Vec at(float u,float v) const { return hip+up*u+fwd*(v*tf); }
};

// One arm solved into joints. The elbow only bends one way, like a human elbow.
struct Renderer::ArmPose {
    Vec shoulder,elbow,wrist,hand;
    // Elbow flexion in radians: 0 is a straight arm, about 2.5 is a fully folded one.
    float flexion=0;
    Vec forearm() const { return normal(wrist-elbow); }
};

// Landmarks of the head shared by the parts that decorate it.
struct Renderer::Face {
    Vec head,ear,eyeA,eyeB;
    float ft=1,fa=1,look=0;
};
}
