#include "render/renderer.hpp"

namespace vl {
Vec Renderer::platformRear(Vec edge,float depth) const {
    const Vec vanishing{width*.5f+cameraView.tilt.x*340,height*.36f-cameraView.tilt.y*72};
    return edge+(vanishing-edge)*depth;
}

void Renderer::material(const Game& g) {
    flush();
    vertices.clear();
    // Draw rear edges before front panels so the walkable lip remains readable.
    for (size_t i=0;i<g.platforms.size();++i) {
        const auto& surface=g.platforms[i];
        const auto b=platformBox(g,i);
        if (!visible(b.x,b.w)) continue;
        const float depth=surface.ground?.108f:surface.cover?.036f:surface.lift?.071f:.079f;
        const Vec left{b.x+offset.x,b.y+offset.y},right=left+Vec{b.w,0};
        const Vec rearLeft=platformRear(left,depth),rearRight=platformRear(right,depth);
        const Color steel=hex(g.stage->biome==Biome::forge?0x756058:
            g.stage->biome==Biome::biodome?0x5b7166:0x586f83);
        auto face=[&](Vertex a,Vertex c,Vertex d,Vertex e) {
            vertices.insert(vertices.end(),{a,c,d,a,d,e});
        };
        face({rearLeft,{b.x,25},steel.energy(.69f).alpha(.42f)},
             {rearRight,{b.right(),25},steel.energy(.69f).alpha(.42f)},
             {right,{b.right(),1},steel.alpha(.68f)},
             {left,{b.x,1},steel.alpha(.68f)});
        const Color edge=steel.energy(.47f).alpha(.28f);
        const Vec drop{0,b.h};
        if (rearLeft.x<left.x)
            face({rearLeft,{b.x-20,4},edge},{left,{b.x,4},edge.energy(1.22f)},
                 {left+drop,{b.x,b.h},edge.energy(.60f)},{rearLeft+drop,{b.x-20,b.h},edge.energy(.52f)});
        if (rearRight.x>right.x)
            face({right,{b.right(),4},edge.energy(1.22f)},{rearRight,{b.right()+20,4},edge},
                 {rearRight+drop,{b.right()+20,b.h},edge.energy(.52f)},{right+drop,{b.right(),b.h},edge.energy(.60f)});
    }
    for (size_t i=0;i<g.platforms.size();++i) {
        const auto& surface=g.platforms[i];
        const auto b=platformBox(g,i);
        if (surface.cover || !visible(b.x,b.w)) continue;
        const Color tint=hex(g.stage->biome==Biome::forge?0x69524a:g.stage->biome==Biome::water?0x4c686b:g.stage->biome==Biome::biodome?0x525e4f:g.stage->biome==Biome::orbital?0x4c5772:0x515d64);
        const Rect r{b.x+offset.x,b.y+offset.y,b.w,b.h};
        const float bevel=surface.ground?16.f:5.f;
        const float depth=surface.ground?.58f:.68f;
        const Color top=tint.energy(.95f).alpha(1);
        const Color bottom=tint.energy(depth).alpha(surface.ground?.27f:.48f);
        auto face=[&](float start,float end,Color a,Color z) {
            const float u=b.x;
            const Vertex v0{{r.x,r.y+start},{u,start},a},v1{{r.right(),r.y+start},{u+r.w,start},a};
            const Vertex v2{{r.right(),r.y+end},{u+r.w,end},z},v3{{r.x,r.y+end},{u,end},z};
            vertices.insert(vertices.end(),{v0,v1,v2,v0,v2,v3});
        };
        face(0,bevel,top,top.energy(.69f));
        face(bevel,r.h,top.energy(.71f).alpha(.45f),bottom.energy(.55f));
    }
    gl.UseProgram(materialProgram);
    gl.Uniform1f(materialCamera,g.camera);
    gl.Uniform1i(materialSector,static_cast<int>(g.stage->biome));
    gl.Uniform1i(materialDetail,graphics.materials);
    gl.BindVertexArray(vao); gl.BindBuffer(GL_ARRAY_BUFFER,buffer);
    gl.BufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    vertices.clear();
}
} // namespace vl
