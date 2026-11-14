#pragma once
namespace vl::shader {
inline constexpr auto light=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform vec4 lightData;
uniform vec3 lightColor;
uniform vec2 direction;
uniform float cone;
uniform int blockers,soft;
uniform vec4 occluders[22];

float hitBox(vec2 start,vec2 stop,vec4 box) {
    vec2 d=stop-start;
    vec2 inv=vec2(d.x<0.0?-1.0:1.0,d.y<0.0?-1.0:1.0)/max(abs(d),vec2(0.001));
    vec2 a=(box.xy-start)*inv;
    vec2 b=(box.zw-start)*inv;
    vec2 near=min(a,b),far=max(a,b);
    float n=max(near.x,near.y), f=min(far.x,far.y);
    return f>=max(n,0.018) && n<0.95 ? 0.0 : 1.0;
}
void main() {
    vec2 p=vec2(uv.x,1.0-uv.y)*vec2(1200.0,600.0);
    vec2 start=lightData.xy;
    vec2 d=p-start;
    float r=length(d)/lightData.z;
    if (r>=1.0) discard;
    vec2 forward=d/max(length(d),1.0);
    float spot=cone<0.0?1.0:smoothstep(cone,min(1.0,cone+0.29),dot(forward,normalize(direction)));
    if (spot<=0.0) discard;
    float visibility=1.0;
    vec2 edge=vec2(-forward.y,forward.x)*(2.5+r*7.0);
    for (int i=0;i<blockers;i++) {
        vec4 box=occluders[i];
        if (all(greaterThan(p,box.xy)) && all(lessThan(p,box.zw))) {
            visibility*=0.70;
            continue;
        }
        float amount=hitBox(start,p,box);
        if (soft==1) amount=(amount*2.0+hitBox(start+edge,p,box)+hitBox(start-edge,p,box))*.25;
        visibility*=mix(.10,1.0,amount);
        if (visibility<0.028) break;
    }
    float falloff=(1.0-r*r);
    falloff=falloff*falloff/(1.0+5.8*r*r);
    frag=vec4(lightColor*(lightData.w*falloff*spot*visibility),1.0);
})GLSL";

inline constexpr auto atmosphere=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D world,lighting;
uniform float time,camera,fogStrength;
uniform vec2 lightTexel,worldTexel;
uniform float relief;
uniform vec3 haze;
float luma(vec3 c) { return dot(c,vec3(.299,.587,.114)); }
float hash(vec2 p) {
    return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);
}
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
void main() {
    vec3 raw=texture(world,uv).rgb;
    vec3 light=texture(lighting,uv).rgb;
    vec3 lightX=texture(lighting,uv+vec2(lightTexel.x*3.0,0)).rgb;
    vec3 lightY=texture(lighting,uv+vec2(0,lightTexel.y*4.0)).rgb;
    vec3 scatter=(light*2.0+lightX+lightY)*.25;
    float y=1.0-uv.y;
    vec2 p=vec2(uv.x*7.5+camera*.006,y*8.0);
    float bank=noise(p+vec2(time*.095,0))*.64+noise(p*2.14+vec2(-time*.115,time*.052))*.36;
    float thin=noise(p*vec2(.53,3.2)+vec2(-time*.10,0.0));
    float heightFog=smoothstep(.47,1.07,y+bank*.20);
    float density=(heightFog*.59+.09)*(.28+bank*.89+thin*.24)*fogStrength;
    float transmittance=exp(-density);
    // Relief: the painted backplate's luminance acts as a height field and the gradient of the light map
    // points toward the light, so lit faces catch light while cavities and far sides stay dark.
    if (relief<.01) {
        vec3 base=raw*.84+light*(raw*.92+.047);
        frag=vec4(base*transmittance+haze*(.2+scatter*.44)*(1.0-transmittance),1.0);
        return;
    }
    vec2 tx=worldTexel*1.5, lt=lightTexel*2.0;
    vec2 slope=vec2(luma(texture(world,uv+vec2(tx.x,0)).rgb)-luma(texture(world,uv-vec2(tx.x,0)).rgb),
                    luma(texture(world,uv+vec2(0,tx.y)).rgb)-luma(texture(world,uv-vec2(0,tx.y)).rgb));
    vec2 toward=vec2(luma(texture(lighting,uv+vec2(lt.x,0)).rgb)-luma(texture(lighting,uv-vec2(lt.x,0)).rgb),
                     luma(texture(lighting,uv+vec2(0,lt.y)).rgb)-luma(texture(lighting,uv-vec2(0,lt.y)).rgb));
    vec3 n=normalize(vec3(-slope*relief,1.0));
    vec3 l=normalize(vec3(toward*46.0,.34));
    float facing=dot(n,l)-l.z;
    vec3 shaped=light*clamp(1.0+facing*1.9,.25,2.1);
    vec3 base=raw*(.84+shaped*.92)+shaped*.047;
    vec3 inScatter=haze*(.20+scatter*.44)+scatter*.087;
    frag=vec4(base*transmittance+inScatter*(1.0-transmittance),1.0);
})GLSL";
}
