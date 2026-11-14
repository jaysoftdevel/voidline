#pragma once
namespace vl::shader {
inline constexpr auto cinematic=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D image,incoming;
uniform float time,fade,dissolve,depth;
uniform vec3 tint;
uniform vec2 focus;
float grain(vec2 p) {
    vec3 q=fract(vec3(p.xyx)*.1031);
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
vec3 scene(vec2 p) {
    if (dissolve>=1.) return texture(image,p).rgb;
    return mix(texture(image,p).rgb,texture(incoming,p).rgb,dissolve);
}
void main() {
    vec3 base=scene(uv);
    vec3 glow=vec3(0);
    vec3 soft=vec3(0);
    const vec2 stepUV=vec2(.0055,.010);
    for (int i=0;i<6;++i) {
        float angle=float(i)*1.04719755;
        vec3 tap=scene(uv+vec2(cos(angle),sin(angle))*stepUV);
        glow+=tap*smoothstep(.54,1.24,dot(tap,vec3(.25,.62,.13)));
        soft+=tap;
    }
    vec2 p=uv-.5;
    vec2 outside=(uv-focus)*vec2(1.,.78);
    float falloff=smoothstep(.19,.65,length(outside))*depth;
    base=mix(base,soft/6.,falloff);
    base=mix(base,base*(.79+.34*tint),.16);
    base+=glow*.055;
    base=base/(.88+.13*base);
    base*=1.045-.42*dot(p,p*vec2(.72,1.36));
    float tick=time*24.;
    float speckle=mix(grain(gl_FragCoord.xy+floor(tick)*7.1),grain(gl_FragCoord.xy+ceil(tick)*7.1),fract(tick));
    base+=(speckle-.5)*.012;
    frag=vec4(mix(vec3(.018,.024,.034),max(base,vec3(0)),fade),1);
}
)GLSL";
}
