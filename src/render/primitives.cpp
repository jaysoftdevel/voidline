#include "render/renderer.hpp"

namespace vl {
Vec Renderer::transform(Vec p) const {
    if (!spinning) return p;
    const Vec d=p-spinPivot;
    return spinPivot+Vec{d.x*spinCos-d.y*spinSin,d.x*spinSin+d.y*spinCos};
}

void Renderer::beginSpin(Vec pivot,float angle) {
    spinning=true; spinPivot=pivot; spinCos=std::cos(angle); spinSin=std::sin(angle);
}

void Renderer::endSpin() { spinning=false; }

void Renderer::quad(Rect r,Color top,Color bottom,GLuint id,Rect uv) {
    if (batchTexture!=id) { flush(); batchTexture=id; }
    top=shade(top); bottom=shade(bottom);
    const Vertex a{transform({r.x,r.y}),{uv.x,uv.y},top}, b{transform({r.right(),r.y}),{uv.right(),uv.y},top};
    const Vertex c{transform({r.right(),r.bottom()}),{uv.right(),uv.bottom()},bottom}, d{transform({r.x,r.bottom()}),{uv.x,uv.bottom()},bottom};
    vertices.insert(vertices.end(),{a,b,c,a,c,d});
}

void Renderer::sprite(std::string_view name,Vec anchor,Vec axisU,Vec axisV,Vec anchorUV,Color tint) {
    const auto& t=asset(name);
    if (batchTexture!=t.id) { flush(); batchTexture=t.id; }
    tint=shade(tint);
    auto corner=[&](float u,float v) {
        return Vertex{transform(anchor+axisU*(u-anchorUV.x)+axisV*(v-anchorUV.y)),{u,v},tint};
    };
    const Vertex a=corner(0,0),b=corner(1,0),c=corner(1,1),d=corner(0,1);
    vertices.insert(vertices.end(),{a,b,c,a,c,d});
}

void Renderer::beam(Vec from,float fromWidth,Vec to,float toWidth,Color start,Color end) {
    if (batchTexture!=white.id) { flush(); batchTexture=white.id; }
    const Vec d=normal(to-from), n{-d.y,d.x};
    const Vertex a{from-n*(fromWidth*.5f),{.5f,.5f},start},b{from+n*(fromWidth*.5f),{.5f,.5f},start};
    const Vertex c{to+n*(toWidth*.5f),{.5f,.5f},end},e{to-n*(toWidth*.5f),{.5f,.5f},end};
    vertices.insert(vertices.end(),{a,b,c,a,c,e});
}

void Renderer::puff(Vec center,float radius,float angle,Color color) {
    if (batchTexture!=cloud.id) { flush(); batchTexture=cloud.id; }
    color=shade(color);
    const Vec u=ray(angle)*radius,v=Vec{-u.y,u.x};
    const Vertex a{center-u-v,{0,0},color},b{center+u-v,{1,0},color},c{center+u+v,{1,1},color},d{center-u+v,{0,1},color};
    vertices.insert(vertices.end(),{a,b,c,a,c,d});
}

void Renderer::slab(Rect r,float slant,Color top,Color bottom) {
    if (batchTexture!=white.id) { flush(); batchTexture=white.id; }
    const Vertex a{{r.x+slant,r.y},{.5f,.5f},top},b{{r.right()+slant,r.y},{.5f,.5f},top};
    const Vertex c{{r.right(),r.bottom()},{.5f,.5f},bottom},d{{r.x,r.bottom()},{.5f,.5f},bottom};
    vertices.insert(vertices.end(),{a,b,c,a,c,d});
}

void Renderer::slabOutline(Rect r,float slant,Color color,float weight) {
    const Vec a{r.x+slant,r.y},b{r.right()+slant,r.y},c{r.right(),r.bottom()},d{r.x,r.bottom()};
    line(a,b,weight,color); line(b,c,weight,color); line(c,d,weight,color); line(d,a,weight,color);
}

void Renderer::rect(Rect r,Color color) { quad(r,color,color,white.id); }

void Renderer::gradient(Rect r,Color top,Color bottom) { quad(r,top,bottom,white.id); }

void Renderer::outline(Rect r,Color c,float w) {
    rect({r.x,r.y,r.w,w},c); rect({r.x,r.bottom()-w,r.w,w},c);
    rect({r.x,r.y+w,w,r.h-2*w},c); rect({r.right()-w,r.y+w,w,r.h-2*w},c);
}

void Renderer::triangle(Vec a,Vec b,Vec c,Color color) {
    if (batchTexture!=white.id) { flush(); batchTexture=white.id; }
    color=shade(color);
    vertices.insert(vertices.end(),{{transform(a),{.5f,.5f},color},{transform(b),{.5f,.5f},color},{transform(c),{.5f,.5f},color}});
}

void Renderer::line(Vec a,Vec b,float weight,Color color) {
    Vec d=normal(b-a), off=Vec{-d.y,d.x}*(weight*.5f);
    triangle(a+off,b+off,b-off,color); triangle(a+off,b-off,a-off,color);
}

void Renderer::ellipse(Vec c,Vec r,Color color) {
    constexpr int segments=20;
    for (int i=0;i<segments;++i) {
        const auto a=ray(i*2*pi/segments), b=ray((i+1)*2*pi/segments);
        triangle(c,c+Vec{a.x*r.x,a.y*r.y},c+Vec{b.x*r.x,b.y*r.y},color);
    }
}

void Renderer::halo(Vec p,float radius,Color color) {
    quad({p.x-radius,p.y-radius,radius*2,radius*2},color,color,radial.id);
}

void Renderer::image(std::string_view name,Rect r,Color color) {
    const auto& t=asset(name); quad(r,color,color,t.id);
}

void Renderer::cover(std::string_view name,Rect r,float zoom,Vec bias,Color tint) {
    const auto& t=asset(name);
    const float scale=std::max(r.w/t.w,r.h/t.h)*zoom;
    const float u=r.w/(t.w*scale),v=r.h/(t.h*scale);
    quad(r,tint,tint,t.id,{(1-u)*bias.x,(1-v)*bias.y,u,v});
}
} // namespace vl
