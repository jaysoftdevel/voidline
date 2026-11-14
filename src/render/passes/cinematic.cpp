#include "render/figures/humanoid.hpp"
#include "render/cinematic_gait.hpp"

namespace vl {
void Renderer::beginCinematic(int layer) {
    const int w=std::clamp(int(viewW*resolution),800,software?960:1600)/16*16;
    if (w!=filmWidth) {
        for (auto& frame:filmFrames) {
            if (frame.framebuffer) destroy(frame);
            frame=target(w,w/2);
        }
        filmWidth=w;
    }
    bind(filmFrames[layer],true);
    gl.Enable(GL_BLEND);
    gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::finishCinematic(float time,float fade,Color tint,float dissolve,Vec focus,float depth) {
    output();
    gl.Disable(GL_BLEND);
    gl.UseProgram(cinematicProgram);
    useTexture(cinematicProgram,"image",0,filmFrames[0].texture.id);
    useTexture(cinematicProgram,"incoming",1,filmFrames[1].texture.id);
    gl.Uniform1f(uniform(cinematicProgram,"time"),time);
    gl.Uniform1f(uniform(cinematicProgram,"fade"),fade);
    gl.Uniform1f(uniform(cinematicProgram,"dissolve"),dissolve);
    gl.Uniform2f(uniform(cinematicProgram,"focus"),focus.x,1.f-focus.y);
    gl.Uniform1f(uniform(cinematicProgram,"depth"),depth);
    gl.Uniform3f(uniform(cinematicProgram,"tint"),tint.r,tint.g,tint.b);
    screen(cinematicProgram);
    gl.Enable(GL_BLEND);
    gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::cinematicOperative(const Character& character,const CinematicPose& p) {
    flush();
    figureOrigin=p.feet;
    figureScale=p.scale;
    figureRim=p.rim;
    figureLightSide=p.facing;
    tinting=true;
    actorLight=p.light;
    auto h=operativeFigure(character);
    h.ground=p.feet;
    h.facing=h.stride=p.facing;
    h.weight=cinematicEase(p.walk);
    h.turn=p.facing*mix(p.turn,.97f,h.weight)+.007f*std::sin(p.time*.91f);
    h.time=p.time;
    h.cycle=p.distance/(cinematicStride*p.scale);
    // Weight transfer lags behind the grounded foot, rather than bobbing the
    // entire body independently of translation. Breathing continues at rest.
    const float stride=std::sin(h.cycle*2*pi);
    h.lean=p.facing*(.105f*h.weight+.105f*p.reach+.018f*std::sin(p.time*1.13f));
    h.pelvis=-.95f*stride*stride*h.weight+.14f*std::sin(p.time*1.65f);
    h.aim=normal({float(p.facing),p.look});
    h.velocity={float(p.facing)*h.weight*132,0};
    h.run=.12f*h.weight;
    h.crouch=p.crouch;
    h.cinematic=true;
    h.speech=p.speech;
    // Offset character blinks in time; longer quiet holds retain subtle life.
    h.blink=cinematicEase(1.f-std::abs(std::fmod(p.time+.17f,4.1f)-2.06f)/.14f);
    h.relax=1; h.nearRelax=1-p.reach;
    for (int i=0;i<2;++i) {
        const auto foot=cinematicFoot(h.cycle+i*.5f);
        h.heels[i]=p.feet+Vec{p.facing*mix(i==0?-5.f:5.f,foot.contact+(i==0?-1.f:1.f),h.weight),
                              -foot.lift*h.weight};
        h.footAngle[i]=foot.pitch*h.weight;
    }
    const auto rig=buildRig(h);
    h.nearHand=rig.shoulder+Vec{p.facing*p.handOffset.x,p.handOffset.y};
    h.farHand=rig.farShoulder+Vec{p.facing*8.f,15.f};
    if (!p.carried.empty()) {
        h.weapon=[&](Vec hand,Vec) {
            const float turn=p.facing*.13f;
            sprite(p.carried,hand,Vec{15.f,turn*15},Vec{-turn*6,6},{.50f,0});
        };
    }
    humanoid(h);
    flush();
    tinting=false;
    figureScale=1.f;
}
}
