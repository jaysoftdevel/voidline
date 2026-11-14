#pragma once
namespace vl::shader {
inline constexpr auto mesh=R"GLSL(#version 330 core
layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;
out vec2 vUV;
out vec4 tint;
void main() {
    gl_Position=vec4(position.x/600.0-1.0,1.0-position.y/300.0,0.0,1.0);
    vUV=uv;
    tint=color;
})GLSL";

inline constexpr auto texture=R"GLSL(#version 330 core
in vec2 vUV;
in vec4 tint;
out vec4 frag;
uniform sampler2D image;
void main() { frag=texture(image,vUV)*tint; }
)GLSL";

inline constexpr auto material=R"GLSL(#version 330 core
in vec2 vUV;
in vec4 tint;
out vec4 frag;
uniform float camera;
uniform int sector;
float hash21(vec2 p) {
    vec3 q=fract(p.xyx*vec3(.1031,.1030,.0973));
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
float grain(vec2 p) {
    vec2 i=floor(p),f=smoothstep(0.0,1.0,fract(p));
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
               mix(hash21(i+vec2(0,1)),hash21(i+1.0),f.x),f.y);
}
void main() {
    vec2 p=vUV;
    float gauge=tint.a;
    float scale=max(fwidth(p.x),.72);
    float grit=grain(p*.41)-.5;
    float flakes=grain(p*vec2(.13,.24))-.5;
    float slow=grain(p*.045);
    float brushed=grain(p*vec2(.068,.89))-.5;
    float edge=exp(-max(0.0,p.y)*.46);
    float dent=smoothstep(.60,.84,grain(p*.092+31.0));
    float directional=(grain((p+vec2(1.4,-1.8))*.16)-grain(p*.16))*.17;
    vec3 base=tint.rgb*(.88+grit*.12+flakes*.18+brushed*.10+directional);
    vec3 oxide=sector==1?vec3(.055,.118,.103):sector==2?vec3(.100,.145,.063):
        sector==3?vec3(.187,.083,.037):sector==4?vec3(.068,.084,.13):vec3(.142,.080,.046);
    float corrosion=smoothstep(.49,.77,slow)*mix(.35,.73,dent);
    base=mix(base,oxide*(.74+flakes*.56),corrosion*.73);
    float scar=grain(vec2(p.x*.047+p.y*.12,p.y*.60));
    float scratches=smoothstep(.83,.99,scar)*(.14+edge*.31)*min(1.0,1.30/scale);
    float gleam=(.20+.16*sin((p.x+camera*.16)*.040))*edge;
    base+=vec3(.38,.48,.54)*(scratches+gleam)*gauge;
    frag=vec4(max(vec3(.009),base),1.0);
})GLSL";

inline constexpr auto fullscreen=R"GLSL(#version 330 core
out vec2 uv;
void main() {
    vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
    uv=p;
    gl_Position=vec4(p*2.0-1.0,0.0,1.0);
})GLSL";
}
