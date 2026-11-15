#pragma once
// Shared math for the native four-view skeleton editor. No OpenGL dependency:
// perspective camera and orthographic axes are projected by the SDL renderer.
#include "content/skeleton_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace vl::studio {
struct Point3 {
    float x=0,y=0,z=0;
    constexpr Point3 operator+(Point3 o)const{return {x+o.x,y+o.y,z+o.z};}
    constexpr Point3 operator-(Point3 o)const{return {x-o.x,y-o.y,z-o.z};}
    constexpr Point3 operator*(float t)const{return {x*t,y*t,z*t};}
};
inline float dot(Point3 a,Point3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
enum class View { front,right,top,orbit };
inline constexpr std::array<const char*,4> viewNames{{"FRONT / XY","RIGHT / ZY","TOP / XZ","3D / ORBIT"}};
inline constexpr int frameCount=60;
inline int nextFrame(int frame){return (frame+1)%frameCount;}
struct Camera {
    float yaw=.72f,pitch=.34f,scale=2.7f,panX=0,panY=0;
    void orbit(float dx,float dy){
        yaw+=dx*.012f;
        pitch=std::clamp(pitch+dy*.008f,-1.3f,1.3f);
    }
    void zoom(float dy){scale=std::clamp(scale*(1+dy*.10f),.8f,7.f);}
};
struct Axes {Point3 right,down;};
inline Axes axes(View view,const Camera& camera){
    switch(view){
    case View::front:return {{1,0,0},{0,1,0}};
    case View::right:return {{0,0,1},{0,1,0}};
    case View::top:return {{1,0,0},{0,0,1}};
    case View::orbit:{
        const float cy=std::cos(camera.yaw),sy=std::sin(camera.yaw);
        const float cp=std::cos(camera.pitch),sp=std::sin(camera.pitch);
        return {{cy,0,sy},{-sy*sp,cp,cy*sp}};
    }
    }
    return {};
}
// A preview of the original procedural game rig, not a skinned polygon mesh.
// One discrete animation frame is always deterministic; playback and stepping
// sample exactly the same positions.
inline Point3 joint(const SkeletonProfile& model,SkeletonJoint type,int pose,int frame){
    pose=std::clamp(pose,0,2);
    const size_t j=static_cast<size_t>(type);
    Vec xy=modelJoint(model,type,pose==1?1.f:0.f,pose==2?1.f:0.f);
    Point3 p{xy.x,xy.y,skeletonReferenceDepth[j]+model.depths[0][j]+
        (pose?model.depths[size_t(pose)][j]:0.f)};
    const float phase=6.28318530718f*float((frame%frameCount+frameCount)%frameCount)/float(frameCount);
    const float swing=std::sin(phase),alternate=std::sin(phase+3.14159265359f);
    const float lift=std::max(0.f,std::sin(phase));
    const float oppositeLift=std::max(0.f,alternate);
    if(pose==2){
        const float stride=8.f*model.stride;
        if(type==SkeletonJoint::nearKnee || type==SkeletonJoint::nearAnkle){
            p.x+=stride*swing; p.y-=lift*7.f*model.stride;
        }else if(type==SkeletonJoint::farKnee || type==SkeletonJoint::farAnkle){
            p.x-=stride*swing; p.y-=oppositeLift*7.f*model.stride;
        }else if(type==SkeletonJoint::nearElbow || type==SkeletonJoint::nearHand){
            p.x-=swing*5.f*model.armSwing;
        }else if(type==SkeletonJoint::farElbow || type==SkeletonJoint::farHand){
            p.x+=swing*5.f*model.armSwing;
        }else if(type==SkeletonJoint::head || type==SkeletonJoint::neck || type==SkeletonJoint::waist){
            p.y+=std::cos(phase*2.f)*1.4f;
        }
    }else if(pose==1){
        const float recoil=std::max(0.f,std::sin(phase*2.f))*model.recoil*1.8f;
        if(type==SkeletonJoint::nearHand||type==SkeletonJoint::farHand){p.x-=recoil;p.y-=model.aimRaise*1.5f;}
        if(type==SkeletonJoint::nearElbow||type==SkeletonJoint::farElbow)p.x-=recoil*.55f;
        if(type==SkeletonJoint::nearShoulder||type==SkeletonJoint::farShoulder)p.x-=recoil*.3f;
    }else if(type==SkeletonJoint::head||type==SkeletonJoint::neck||type==SkeletonJoint::waist){
        p.y+=std::sin(phase)*.7f*model.idleBreath;
    }
    return p;
}
inline Vec project(Point3 p,View view,const Camera& camera,Vec center){
    const auto b=axes(view,camera);
    float perspective=1.f;
    if(view==View::orbit){
        // Distance attenuation makes the orbit viewport genuinely perspective,
        // rather than just a rotatable orthographic side view.
        Point3 forward{b.right.y*b.down.z-b.right.z*b.down.y,
                       b.right.z*b.down.x-b.right.x*b.down.z,
                       b.right.x*b.down.y-b.right.y*b.down.x};
        perspective=std::clamp(180.f/(180.f+dot(p,forward)),.65f,1.5f);
    }
    return {center.x+camera.panX+dot(p,b.right)*camera.scale*perspective,
            center.y+camera.panY+dot(p,b.down)*camera.scale*perspective};
}
// Moving a handle acts in the visible camera plane. Front edits XY, side edits
// ZY, top edits XZ; orbit updates all three axes in screen-aligned space.
inline Point3 dragDelta(View view,const Camera& camera,float screenDx,float screenDy){
    const auto b=axes(view,camera);
    const float dx=screenDx/std::max(.05f,camera.scale);
    const float dy=screenDy/std::max(.05f,camera.scale);
    return b.right*dx+b.down*dy;
}
inline void moveJoint(SkeletonProfile& p,int pose,int index,Point3 delta){
    if(index<0||index>=int(skeletonJointCount))return;
    pose=std::clamp(pose,0,2);
    auto& point=p.offsets[size_t(pose)][size_t(index)];
    auto& depth=p.depths[size_t(pose)][size_t(index)];
    point.x=std::clamp(point.x+delta.x,-25.f,25.f);
    point.y=std::clamp(point.y+delta.y,-25.f,25.f);
    depth=std::clamp(depth+delta.z,-25.f,25.f);
}
} // namespace vl::studio
