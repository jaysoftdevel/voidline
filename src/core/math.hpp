#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace vl {
constexpr float width = 1200, height = 600, pi = 3.14159265358979323846f;
constexpr float step = 1.f / 120.f;
struct Vec {
    float x = 0, y = 0;
    Vec operator+(Vec b) const { return {x + b.x, y + b.y}; }
    Vec operator-(Vec b) const { return {x - b.x, y - b.y}; }
    Vec operator*(float n) const { return {x*n, y*n}; }
    Vec& operator+=(Vec b) { x+=b.x; y+=b.y; return *this; }
};
inline float dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y; }
inline float length(Vec v) { return std::hypot(v.x,v.y); }
inline Vec normal(Vec v) { return v * (1.f / std::max(length(v), .0001f)); }
inline Vec ray(float a) { return {std::cos(a),std::sin(a)}; }
inline float clamp(float x, float lo = 0, float hi = 1) { return std::clamp(x,lo,hi); }
inline float mix(float a,float b,float t) { return a+(b-a)*t; }
inline Vec mix(Vec a,Vec b,float t) { return a+(b-a)*t; }
inline float smooth(float t) { t=clamp(t); return t*t*(3-2*t); }
inline float approach(float x,float target,float delta) { return x+clamp(target-x,-delta,delta); }
inline int sign(float x) { return x<0 ? -1 : 1; }
struct Rect {
    float x=0,y=0,w=0,h=0;
    float right() const { return x+w; }
    float bottom() const { return y+h; }
    Vec center() const { return {x+w*.5f,y+h*.5f}; }
    bool contains(Vec p) const { return p.x>=x && p.x<=right() && p.y>=y && p.y<=bottom(); }
    bool overlaps(Rect b) const { return x<b.right() && right()>b.x && y<b.bottom() && bottom()>b.y; }
};
struct Color {
    float r=1,g=1,b=1,a=1;
    Color alpha(float v) const { return {r,g,b,v}; }
    Color energy(float e) const { return {r*e,g*e,b*e,a}; }
};
inline Color mix(Color a,Color b,float t) { return {a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,a.a+(b.a-a.a)*t}; }
inline Color hex(uint32_t c) { return {float((c>>16)&255)/255,float((c>>8)&255)/255,float(c&255)/255,1}; }
inline Color hex(const char* c) {
    auto digit=[](char d){return d<='9' ? d-'0' : (d|32)-'a'+10;};
    uint32_t n=0;
    for (++c;*c;++c) n=n*16+digit(*c);
    return hex(n);
}
}
