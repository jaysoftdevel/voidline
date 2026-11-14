#pragma once
namespace vl::shader {
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
}
