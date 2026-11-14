#pragma once
namespace vl::shader {
inline constexpr auto present=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D world,bloom;
uniform float strength,time,slow,hurt,punch,flash,combo,exposure,zoom;
uniform vec2 focus,perspective,roll;
uniform float overscan;
uniform vec3 shadowTint,highlightTint;
uniform float saturation;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
vec3 sampleWorld(vec2 p) { return texture(world,p).rgb+texture(bloom,p).rgb*strength; }
// AgX display transform (Troy Sobotka, as used by Blender 4) with the polynomial sigmoid approximation by Benjamin Wrensch.
vec3 agxContrast(vec3 x) {
    vec3 x2=x*x, x4=x2*x2;
    return 15.5*x4*x2-40.14*x4*x+31.96*x4-6.868*x2*x+0.4298*x2+0.1191*x-0.00232;
}
vec3 agx(vec3 v) {
    const mat3 toAgx=mat3(0.842479062253094,0.0423282422610123,0.0423756549057051,
                          0.0784335999999992,0.878468636469772,0.0784336,
                          0.0792237451477643,0.0791661274605434,0.879142973793104);
    const mat3 fromAgx=mat3(1.19687900512017,-0.0528968517574562,-0.0529716355144438,
                            -0.0980208811401368,1.15190312990417,-0.0980434501171241,
                            -0.0990297440797205,-0.0989611768448433,1.15107367264116);
    const float minEv=-12.47393,maxEv=4.026069;
    v=toAgx*max(v,vec3(1e-6));
    v=clamp(log2(v),minEv,maxEv);
    v=(v-minEv)/(maxEv-minEv);
    v=agxContrast(v);
    // Punchy look: power and saturation lift.
    float l=dot(v,vec3(.2126,.7152,.0722));
    v=pow(max(v,vec3(0.0)),vec3(1.32));
    l=dot(v,vec3(.2126,.7152,.0722));
    v=l+1.38*(v-l);
    return clamp(fromAgx*v,0.0,1.0);
}
void main() {
    vec2 q=vec2(uv.x,1.0-uv.y)*2.0-1.0;
    vec2 local=vec2(q.x*roll.x-q.y*roll.y*.5,q.x*roll.y*2.0+q.y*roll.x);
    local/=overscan+dot(q,perspective);
    vec2 sampleUV=vec2(local.x,-local.y)*.5+.5;
    vec2 p=focus+(sampleUV-focus)*zoom;
    vec2 radial=(p-.5);
    float fringe=.0007+slow*.0016+punch*.0018+hurt*.0040+combo*.0008;
    vec3 c;
    c.r=sampleWorld(p+radial*fringe*2.0).r;
    c.g=sampleWorld(p).g;
    c.b=sampleWorld(p-radial*fringe*2.0).b;
    c=agx(c*exposure);
    float luma=dot(c,vec3(.299,.587,.114));
    c=mix(vec3(luma),c,saturation*(1.0-slow*.20));
    c=mix(c*shadowTint,c*highlightTint,smoothstep(.12,.72,luma));
    vec3 chill=vec3(.90,.97,1.10);
    c=mix(c,c*chill+vec3(.002,.008,.020),slow);
    vec2 fromFocus=sampleUV-focus;
    fromFocus.x*=2.0;
    float angle=atan(fromFocus.y,fromFocus.x);
    float lines=hash(vec2(floor(angle*170.0),floor(time*16.0)));
    float streak=smoothstep(.90,1.0,lines)*smoothstep(.42,1.0,length(fromFocus))*slow;
    c+=vec3(.55,.80,1.0)*streak*.20;
    vec2 edge=(uv-.5)*vec2(1.0,.77);
    float vignette=dot(edge,edge);
    c*=1.0-(.50+slow*.24)*vignette;
    c+=vec3(1.0,.17,.10)*hurt*smoothstep(.04,.38,vignette)*.75;
    c+=vec3(1.0,.93,.78)*flash*.30;
    frag=vec4(c,1.0);
})GLSL";

inline constexpr auto fxaa=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D image;
uniform vec2 texel;
uniform float time;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
void main() {
    const vec3 lumaWeights=vec3(.299,.587,.114);
    vec3 nw=texture(image,uv+vec2(-1,-1)*texel).rgb, ne=texture(image,uv+vec2(1,-1)*texel).rgb;
    vec3 sw=texture(image,uv+vec2(-1,1)*texel).rgb, se=texture(image,uv+vec2(1,1)*texel).rgb;
    vec3 m=texture(image,uv).rgb;
    float lnw=dot(nw,lumaWeights),lne=dot(ne,lumaWeights),lsw=dot(sw,lumaWeights),lse=dot(se,lumaWeights),lm=dot(m,lumaWeights);
    float lmin=min(lm,min(min(lnw,lne),min(lsw,lse))), lmax=max(lm,max(max(lnw,lne),max(lsw,lse)));
    vec2 dir=vec2(-((lnw+lne)-(lsw+lse)),(lnw+lsw)-(lne+lse));
    float reduce=max((lnw+lne+lsw+lse)*(.25/8.0),1.0/128.0);
    float rcp=1.0/(min(abs(dir.x),abs(dir.y))+reduce);
    dir=clamp(dir*rcp,vec2(-8.0),vec2(8.0))*texel;
    vec3 a=.5*(texture(image,uv+dir*(1.0/3.0-.5)).rgb+texture(image,uv+dir*(2.0/3.0-.5)).rgb);
    vec3 b=a*.5+.25*(texture(image,uv+dir*-.5).rgb+texture(image,uv+dir*.5).rgb);
    float lb=dot(b,lumaWeights);
    vec3 c=(lb<lmin||lb>lmax)?a:b;
    c+=hash(gl_FragCoord.xy+fract(time)*61.0)*.016-.008;
    frag=vec4(c,1.0);
})GLSL";
}
