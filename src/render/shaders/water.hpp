#pragma once
namespace vl::shader {
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
}
