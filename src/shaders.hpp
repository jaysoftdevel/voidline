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
inline constexpr auto water=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D reflection;
uniform float surface,time,camera,reflectivity;
uniform vec3 tint;
void main() {
    float y=(1.0-uv.y)*600.0;
    if (y<surface) discard;
    float depth=y-surface;
    float x=uv.x*1200.0+camera;
    float ripple=sin(x*.042+time*2.6+depth*.20)+sin(x*.017-time*1.9-depth*.43)*.6;
    float blur=depth*.000037;
    vec2 mirrored=vec2(uv.x+ripple*(.00085+depth*.000045),1.0-(2.0*surface-y)/600.0);
    vec3 reflected=texture(reflection,mirrored).rgb*.54+
        (texture(reflection,mirrored+vec2(blur,.0015)).rgb+texture(reflection,mirrored-vec2(blur,.0015)).rgb)*.23;
    float fresnel=exp(-depth*.019)*reflectivity;
    float bands=pow(max(0.0,sin(depth*.63+ripple*.62-time*2.1)),9.0)*exp(-depth*.028);
    float glint=pow(max(0.0,sin(x*.040+sin(x*.018-time*1.5)+depth*.58-time*3.1)),15.0);
    bands+=glint*.41*exp(-depth*.051);
    float edge=exp(-depth*.95)*(.68+.32*sin(x*.06+time*1.5));
    vec3 body=tint*(.098+bands*.23+edge*.61);
    frag=vec4(body+reflected*fresnel,1.0);
})GLSL";
inline constexpr auto atmosphere=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D world,lighting;
uniform float time,camera,fogStrength;
uniform vec2 lightTexel;
uniform vec3 haze;
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
    vec3 base=raw*(.84+light*.92)+light*.047;
    vec3 inScatter=haze*(.20+scatter*.44)+scatter*.087;
    frag=vec4(base*transmittance+inScatter*(1.0-transmittance),1.0);
})GLSL";
inline constexpr auto down=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D source;
uniform vec2 texel;
uniform int first;
void main() {
    vec3 c=texture(source,uv).rgb*.36;
    c+=(texture(source,uv+texel*vec2(1,0)).rgb+texture(source,uv+texel*vec2(-1,0)).rgb+
        texture(source,uv+texel*vec2(0,1)).rgb+texture(source,uv+texel*vec2(0,-1)).rgb)*.10;
    c+=(texture(source,uv+texel*vec2(1,1)).rgb+texture(source,uv+texel*vec2(-1,1)).rgb+
        texture(source,uv+texel*vec2(1,-1)).rgb+texture(source,uv+texel*vec2(-1,-1)).rgb)*.06;
    float v=max(c.r,max(c.g,c.b));
    if (first==1) {
        float knee=clamp(v-.61,0.0,.70);
        c*=max(v-.96,knee*knee/1.4)/max(v,.001);
    }
    frag=vec4(c,1.0);
})GLSL";
inline constexpr auto up=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D source;
uniform vec2 texel;
void main() {
    vec3 v=vec3(0.0);
    for (int y=-1;y<=1;++y) for (int x=-1;x<=1;++x) {
        float k=float((2-abs(x))*(2-abs(y)));
        v+=texture(source,uv+vec2(x,y)*texel*.76).rgb*k;
    }
    frag=vec4(v*.034,1.0);
})GLSL";
inline constexpr auto present=R"GLSL(#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D world,bloom;
uniform float strength,time,slow,hurt,punch,flash,combo;
uniform vec2 focus;
uniform vec3 shadowTint,highlightTint;
uniform float saturation;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
vec3 sampleWorld(vec2 p) { return texture(world,p).rgb+texture(bloom,p).rgb*strength; }
void main() {
    // A short zoom punch and a slow-motion push-in, centered on the operative.
    float zoom=1.0-punch*.030-slow*.055;
    vec2 centered=uv-focus;
    vec2 p=focus+centered*zoom;
    vec2 radial=(p-.5);
    float fringe=.0007+slow*.0016+punch*.0018+hurt*.0040+combo*.0008;
    vec3 c;
    c.r=sampleWorld(p+radial*fringe*2.0).r;
    c.g=sampleWorld(p).g;
    c.b=sampleWorld(p-radial*fringe*2.0).b;
    // Filmic shoulder with a vivid, comic-book grade.
    vec3 toe=pow(max(c,vec3(0.0)),vec3(.86));
    vec3 mapped=toe*(1.72*toe+.17)/(toe*(1.83*toe+.53)+.18);
    c=mix(toe/(1.0+toe*.47),mapped,.26);
    float luma=dot(c,vec3(.299,.587,.114));
    c=mix(vec3(luma),c,saturation*(1.0-slow*.20));
    c=mix(c,c*c*(3.0-2.0*c),.34);
    c=mix(c*shadowTint,c*highlightTint,smoothstep(.12,.72,luma));
    // Slow motion cools and darkens the frame and streaks it from the edges.
    vec3 chill=vec3(.90,.97,1.10);
    c=mix(c,c*chill+vec3(.002,.008,.020),slow);
    vec2 fromFocus=uv-focus;
    fromFocus.x*=2.0;
    float angle=atan(fromFocus.y,fromFocus.x);
    float lines=hash(vec2(floor(angle*170.0),floor(time*16.0)));
    float streak=smoothstep(.90,1.0,lines)*smoothstep(.42,1.0,length(fromFocus))*slow;
    c+=vec3(.55,.80,1.0)*streak*.20;
    // Edge darkening, damage and kill flashes.
    vec2 q=(uv-.5)*vec2(1.0,.77);
    float vignette=dot(q,q);
    c*=1.0-(.52+slow*.24)*vignette;
    c+=vec3(1.0,.17,.10)*hurt*smoothstep(.04,.38,vignette)*.75;
    c+=vec3(1.0,.93,.78)*flash*.30;
    c+=hash(gl_FragCoord.xy+fract(time)*61.0)*.014-.007;
    frag=vec4(c,1.0);
})GLSL";
}
