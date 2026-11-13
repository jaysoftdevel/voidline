#include "renderer.hpp"
#include "shaders.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>

extern "C" {
unsigned char* WebPDecodeRGBA(const unsigned char*,size_t,int*,int*);
void WebPFree(void*);
struct PngImage {
    void* opaque; uint32_t version,width,height,format,flags,colormapEntries,warningOrError; char message[64];
};
int png_image_begin_read_from_file(PngImage*,const char*);
int png_image_finish_read(PngImage*,const void*,void*,int,void*);
void png_image_free(PngImage*);
}

namespace vl {
void GL::load() {
#define VL_LOAD(result,name,args) name=reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress("gl" #name)); \
    if (!name) throw std::runtime_error("Missing OpenGL entry point gl" #name);
    VL_GL_FUNCTIONS(VL_LOAD)
#undef VL_LOAD
}
GLuint Renderer::program(const char* vertex,const char* fragment) {
    auto compile=[&](GLenum stage,const char* code) {
        GLuint shader=gl.CreateShader(stage);
        gl.ShaderSource(shader,1,&code,nullptr); gl.CompileShader(shader);
        int ok=0; gl.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if (!ok) {
            char log[4096]{}; gl.GetShaderInfoLog(shader,sizeof(log),nullptr,log);
            gl.DeleteShader(shader);
            throw std::runtime_error(std::string("Shader compilation failed: ")+log);
        }
        return shader;
    };
    const GLuint vs=compile(GL_VERTEX_SHADER,vertex), fs=compile(GL_FRAGMENT_SHADER,fragment);
    const GLuint result=gl.CreateProgram();
    gl.AttachShader(result,vs); gl.AttachShader(result,fs); gl.LinkProgram(result);
    gl.DeleteShader(vs); gl.DeleteShader(fs);
    int ok=0; gl.GetProgramiv(result,GL_LINK_STATUS,&ok);
    if (!ok) {
        char log[4096]{}; gl.GetProgramInfoLog(result,sizeof(log),nullptr,log);
        gl.DeleteProgram(result);
        throw std::runtime_error(std::string("Shader linking failed: ")+log);
    }
    return result;
}
Renderer::Renderer(SDL_Window* win,std::filesystem::path art)
    :window(win),assets(std::move(art)) {
    gl.load();
    drawProgram=program(shader::mesh,shader::texture);
    materialProgram=program(shader::mesh,shader::material);
    materialCamera=uniform(materialProgram,"camera");
    materialSector=uniform(materialProgram,"sector");
    lightProgram=program(shader::fullscreen,shader::light);
    waterProgram=program(shader::fullscreen,shader::water);
    atmosphereProgram=program(shader::fullscreen,shader::atmosphere);
    downProgram=program(shader::fullscreen,shader::down);
    upProgram=program(shader::fullscreen,shader::up);
    presentProgram=program(shader::fullscreen,shader::present);
    gl.GenVertexArrays(1,&vao); gl.BindVertexArray(vao);
    gl.GenBuffers(1,&buffer); gl.BindBuffer(GL_ARRAY_BUFFER,buffer);
    gl.EnableVertexAttribArray(0); gl.VertexAttribPointer(0,2,GL_FLOAT,0,sizeof(Vertex),nullptr);
    gl.EnableVertexAttribArray(1); gl.VertexAttribPointer(1,2,GL_FLOAT,0,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,uv)));
    gl.EnableVertexAttribArray(2); gl.VertexAttribPointer(2,4,GL_FLOAT,0,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,tint)));
    const uint8_t whitePixel[]{255,255,255,255};
    white=texture(1,1,whitePixel);
    std::vector<uint8_t> haloPixels(64*64*4,255);
    for (int y=0;y<64;++y) for (int x=0;x<64;++x) {
        const float d=clamp(std::hypot((x-31.5f)/31.5f,(y-31.5f)/31.5f));
        haloPixels[(y*64+x)*4+3]=std::lround((1-d*d)*std::exp(-d*d*4.4f)*255);
    }
    radial=texture(64,64,haloPixels.data());
    for (size_t i=0;i<fonts.size();++i) {
        const auto base=assets/"fonts"/(i==0?"font-regular":"font-black");
        fonts[i].texture=readTexture(base.string()+".png");
        std::ifstream in(base.string()+".txt");
        for (auto& v:fonts[i].advances) in>>v;
        if (!in) throw std::runtime_error("Missing or invalid font metrics: "+base.string());
    }
    raw=target(1200,600); mirror=target(600,300); lightMap=target(480,240); lit=target(1200,600);
    int w=600,h=300;
    for (auto& level:bloom) { level=target(w,h); w/=2; h/=2; }
    vertices.reserve(28000); lights.reserve(64); blockers.reserve(50);
    check();
}
Renderer::~Renderer() {
    for (auto& t:bloom) destroy(t);
    destroy(raw); destroy(mirror); destroy(lightMap); destroy(lit);
    gl.DeleteTextures(1,&white.id); gl.DeleteTextures(1,&radial.id);
    for (auto& font:fonts) gl.DeleteTextures(1,&font.texture.id);
    for (auto& [name,t]:images) gl.DeleteTextures(1,&t.id);
    for (GLuint id:{drawProgram,lightProgram,waterProgram,atmosphereProgram,downProgram,upProgram,presentProgram,materialProgram}) gl.DeleteProgram(id);
    gl.DeleteBuffers(1,&buffer); gl.DeleteVertexArrays(1,&vao);
}
Renderer::Texture Renderer::texture(int w,int h,const void* pixels,GLenum format) {
    Texture t{0,w,h};
    gl.GenTextures(1,&t.id); gl.BindTexture(GL_TEXTURE_2D,t.id);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl.TexImage2D(GL_TEXTURE_2D,0,format,w,h,0,GL_RGBA,format==GL_RGBA16F?GL_FLOAT:GL_UNSIGNED_BYTE,pixels);
    return t;
}
Renderer::Texture Renderer::readTexture(const std::filesystem::path& path) {
    gl.ActiveTexture(GL_TEXTURE0);
    if (path.extension()==".png") {
        PngImage info{}; info.version=1;
        if (!png_image_begin_read_from_file(&info,path.c_str())) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+path.string()+": "+error);
        }
        info.format=3;
        std::vector<uint8_t> bytes(size_t(info.width)*info.height*4);
        if (!png_image_finish_read(&info,nullptr,bytes.data(),0,nullptr)) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+path.string()+": "+error);
        }
        const auto result=texture(info.width,info.height,bytes.data());
        png_image_free(&info);
        return result;
    }
    std::ifstream in(path,std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in),{}};
    int w=0,h=0;
    auto* rgba=WebPDecodeRGBA(bytes.data(),bytes.size(),&w,&h);
    if (!rgba) throw std::runtime_error("Could not decode "+path.string());
    const auto result=texture(w,h,rgba);
    WebPFree(rgba);
    return result;
}
Renderer::Texture& Renderer::asset(std::string_view name) {
    std::string key(name);
    const auto it=images.find(key);
    if (it!=images.end()) return it->second;
    auto path=assets/key;
    if (!path.has_extension()) path+=".webp";
    return images.emplace(key,readTexture(path)).first->second;
}
Renderer::Target Renderer::target(int w,int h) {
    Target t{texture(w,h,nullptr,GL_RGBA16F),0};
    gl.GenFramebuffers(1,&t.framebuffer); gl.BindFramebuffer(GL_FRAMEBUFFER,t.framebuffer);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,t.texture.id,0);
    if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("OpenGL HDR framebuffer is incomplete");
    return t;
}
void Renderer::destroy(Target& t) {
    gl.DeleteFramebuffers(1,&t.framebuffer);
    gl.DeleteTextures(1,&t.texture.id);
}
void Renderer::bind(const Target& t,bool clear) {
    flush(); gl.BindFramebuffer(GL_FRAMEBUFFER,t.framebuffer);
    gl.Viewport(0,0,t.texture.w,t.texture.h);
    if (clear) { gl.ClearColor(0,0,0,1); gl.Clear(GL_COLOR_BUFFER_BIT); }
}
void Renderer::output() {
    flush(); gl.BindFramebuffer(GL_FRAMEBUFFER,0);
    gl.Viewport(viewX,viewY,viewW,viewH);
}
void Renderer::begin() {
    static const auto epoch=std::chrono::steady_clock::now();
    clockTime=std::chrono::duration<float>(std::chrono::steady_clock::now()-epoch).count();
    SDL_GL_GetDrawableSize(window,&pixelW,&pixelH);
    if (pixelW<=0 || pixelH<=0) pixelW=pixelH=1;
    viewW=std::max(1,std::min(pixelW,pixelH*2)); viewH=std::max(1,std::min(pixelH,pixelW/2));
    viewX=(pixelW-viewW)/2; viewY=(pixelH-viewH)/2;
    output();
    gl.ClearColor(.008f,.012f,.022f,1); gl.Clear(GL_COLOR_BUFFER_BIT);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}
void Renderer::end() { flush(); SDL_GL_SwapWindow(window); }
void Renderer::flush() {
    if (vertices.empty()) return;
    gl.UseProgram(drawProgram);
    useTexture(drawProgram,"image",0,batchTexture);
    gl.BindVertexArray(vao); gl.BindBuffer(GL_ARRAY_BUFFER,buffer);
    gl.BufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    vertices.clear();
}
void Renderer::useTexture(GLuint shader,const char* name,int slot,GLuint id) {
    gl.ActiveTexture(GL_TEXTURE0+slot); gl.BindTexture(GL_TEXTURE_2D,id); gl.Uniform1i(uniform(shader,name),slot);
}
void Renderer::screen(GLuint shader) {
    gl.UseProgram(shader); gl.BindVertexArray(vao); gl.DrawArrays(GL_TRIANGLES,0,3);
}
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
    const Vertex a{transform({r.x,r.y}),{uv.x,uv.y},top}, b{transform({r.right(),r.y}),{uv.right(),uv.y},top};
    const Vertex c{transform({r.right(),r.bottom()}),{uv.right(),uv.bottom()},bottom}, d{transform({r.x,r.bottom()}),{uv.x,uv.bottom()},bottom};
    vertices.insert(vertices.end(),{a,b,c,a,c,d});
}
void Renderer::sprite(std::string_view name,Vec anchor,Vec axisU,Vec axisV,Vec anchorUV,Color tint) {
    const auto& t=asset(name);
    if (batchTexture!=t.id) { flush(); batchTexture=t.id; }
    auto corner=[&](float u,float v) {
        return Vertex{transform(anchor+axisU*(u-anchorUV.x)+axisV*(v-anchorUV.y)),{u,v},tint};
    };
    const Vertex a=corner(0,0),b=corner(1,0),c=corner(1,1),d=corner(0,1);
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
void Renderer::outlinedText(Vec p,std::string_view value,float size,Color color,Color ink) {
    const float w=std::max(1.4f,size*.075f);
    for (const Vec o:{Vec{-w,0},Vec{w,0},Vec{0,-w},Vec{0,w},Vec{-w,-w},Vec{w,-w},Vec{-w,w},Vec{w,w}})
        text(p+o,value,size,ink.alpha(color.a),true);
    text(p,value,size,color,true);
}
void Renderer::rect(Rect r,Color color) { quad(r,color,color,white.id); }
void Renderer::gradient(Rect r,Color top,Color bottom) { quad(r,top,bottom,white.id); }
void Renderer::outline(Rect r,Color c,float w) {
    rect({r.x,r.y,r.w,w},c); rect({r.x,r.bottom()-w,r.w,w},c);
    rect({r.x,r.y+w,w,r.h-2*w},c); rect({r.right()-w,r.y+w,w,r.h-2*w},c);
}
void Renderer::triangle(Vec a,Vec b,Vec c,Color color) {
    if (batchTexture!=white.id) { flush(); batchTexture=white.id; }
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
float Renderer::measure(std::string_view value,float size,bool bold) const {
    const auto& font=fonts[bold?1:0];
    float total=0;
    for (unsigned char c:value) if (c<127 && c>=32) total+=font.advances[c-32];
    return total*(size/48);
}
void Renderer::text(Vec p,std::string_view value,float size,Color color,bool bold) {
    const auto& font=fonts[bold?1:0];
    const float scale=size/48;
    for (unsigned char c:value) {
        if (c<32 || c>=127) continue;
        const int glyph=c-32;
        if (c!=' ') quad({p.x-3*scale,p.y-49*scale,54*scale,65*scale},color,color,font.texture.id,
                         {float(glyph%16)/16,float(glyph/16)/6,1.f/16,1.f/6});
        p.x+=font.advances[glyph]*scale;
    }
}
Vec Renderer::mousePosition(int x,int y) const {
    int w=1,h=1; SDL_GetWindowSize(window,&w,&h);
    return {(x*float(pixelW)/std::max(1,w)-viewX)*width/std::max(1,viewW),
            (y*float(pixelH)/std::max(1,h)-(pixelH-viewY-viewH))*height/std::max(1,viewH)};
}
void Renderer::screenshot(const std::filesystem::path& file) {
    flush();
    std::vector<uint8_t> bytes(size_t(pixelW)*pixelH*4);
    gl.ReadPixels(0,0,pixelW,pixelH,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());
    std::ofstream out(file,std::ios::binary);
    out<<"P6\n"<<pixelW<<" "<<pixelH<<"\n255\n";
    for (int y=pixelH-1;y>=0;--y) for (int x=0;x<pixelW;++x) out.write(reinterpret_cast<const char*>(&bytes[(y*pixelW+x)*4]),3);
    if (!out) throw std::runtime_error("Could not save screenshot "+file.string());
}
std::string Renderer::driver() const {
    return std::string(reinterpret_cast<const char*>(gl.GetString(GL_RENDERER)))+" / "+reinterpret_cast<const char*>(gl.GetString(GL_VERSION));
}
void Renderer::check() {
    if (const auto error=gl.GetError()) throw std::runtime_error("OpenGL error "+std::to_string(error));
}

void Renderer::drawWorld(const Game& g,bool high,bool motion,float alpha,const Player& previous,float previousCamera) {
    const float shake=motion?g.shake:0.f;
    offset={-mix(previousCamera,g.camera,alpha)+std::cos(g.time*119)*shake*.55f,std::sin(g.time*97)*shake*.38f};
    const Vec position{g.player.box.x,g.player.box.y};
    const Vec prior{previous.box.x,previous.box.y};
    const float blend=length(prior-position)>30.f?1.f:alpha;
    playerShift=(prior-position)*(1-blend);
    for (int i=0;i<2;++i) {
        drawHeels[i]=mix(previous.gait.feet[i].heel,g.player.gait.feet[i].heel,blend);
        drawFootAngles[i]=mix(previous.gait.feet[i].angle,g.player.gait.feet[i].angle,blend);
    }
    pose=g.player.gait;
    const auto lowReady=[](const Player& p) { return p.sprinting && !p.aiming ? p.gait.run : 0.f; };
    carry=mix(lowReady(previous),lowReady(g.player),blend);
    pose.lean=mix(previous.gait.lean,pose.lean,blend);
    pose.crouch=mix(previous.gait.crouch,pose.crouch,blend);
    pose.pelvis=mix(previous.gait.pelvis,pose.pelvis,blend);
    pose.compression=mix(previous.gait.compression,pose.compression,blend);
    pose.tuck=mix(previous.gait.tuck,pose.tuck,blend);
    pose.reach=mix(previous.gait.reach,pose.reach,blend);
    const int targetW=high?std::clamp(viewW,1200,1920)/16*16:1200;
    const int targetH=targetW/2;
    if (raw.texture.w!=targetW) {
        destroy(raw); destroy(lit); destroy(mirror);
        raw=target(targetW,targetH); lit=target(targetW,targetH);
        mirror=target(targetW/2,targetH/2);
    }
    // The reflection samples its own target, avoiding feedback from the water surface.
    bind(mirror,true); gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    backdrop(g); depthScenery(g); scenery(g); terrain(g); objects(g); actors(g);
    bind(raw,true);
    backdrop(g); depthScenery(g); scenery(g);
    water(g,high);
    terrain(g); objects(g); drawParticles(g); foreground(g);
    flush();
    collectLights(g);
    drawLights(high);
    post(g,high,motion);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& m:g.messages) {
        const Vec p=m.p+offset;
        const float age=m.maxLife-m.life;
        const float size=(m.text.size()>11?15.f:20.f)*(1+.55f*std::exp(-age*14));
        outlinedText({p.x-measure(m.text,size,true)*.5f,p.y},m.text,size,m.color.alpha(clamp(m.life*4)));
    }
}
void Renderer::material(const Game& g) {
    flush();
    vertices.clear();
    for (const auto& surface:g.platforms) {
        const auto& b=surface.box;
        if (!visible(b.x,b.w)) continue;
        const Color tint=hex(g.index==3?0x69524a:g.index==1?0x4c686b:g.index==2?0x525e4f:g.index==4?0x4c5772:0x515d64);
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
    gl.Uniform1i(materialSector,g.index);
    gl.BindVertexArray(vao); gl.BindBuffer(GL_ARRAY_BUFFER,buffer);
    gl.BufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    vertices.clear();
}
void Renderer::water(const Game& g,bool high) {
    flush();
    gl.Disable(GL_BLEND); gl.UseProgram(waterProgram);
    useTexture(waterProgram,"reflection",0,mirror.texture.id);
    constexpr float levels[]{535,523,533,531,532};
    constexpr float reflect[]{.37f,.76f,.47f,.53f,.42f};
    const Color tint=hex(g.index==3?g.stage->accent:g.stage->cold);
    gl.Uniform1f(uniform(waterProgram,"surface"),levels[g.index]+offset.y);
    gl.Uniform1f(uniform(waterProgram,"time"),g.time);
    gl.Uniform1f(uniform(waterProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(waterProgram,"reflectivity"),reflect[g.index]*(high?1.f:.76f));
    gl.Uniform3f(uniform(waterProgram,"tint"),tint.r,tint.g,tint.b);
    screen(waterProgram);
    gl.Enable(GL_BLEND);
}
void Renderer::drawLights(bool high) {
    bind(lightMap,true);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_ONE,GL_ONE); gl.Enable(GL_SCISSOR_TEST);
    gl.UseProgram(lightProgram);
    const int w=lightMap.texture.w,h=lightMap.texture.h;
    const int count=std::min<int>(lights.size(),high?34:16);
    for (int i=0;i<count;++i) {
        const auto& l=lights[i];
        const float r=l.radius;
        const Rect bounds{l.p.x-r,l.p.y-r,r*2,r*2};
        if (!bounds.overlaps({0,0,width,height})) continue;
        const int x0=std::max(0,int(std::floor(bounds.x*w/width))), x1=std::min(w,int(std::ceil(bounds.right()*w/width)));
        const int y0=std::max(0,int(std::floor((height-bounds.bottom())*h/height))), y1=std::min(h,int(std::ceil((height-bounds.y)*h/height)));
        gl.Scissor(x0,y0,x1-x0,y1-y0);
        std::array<float,22*4> data{};
        int n=0;
        if (l.casts) for (const auto& b:blockers) {
            if (n>=22 || !bounds.overlaps(b) || b.contains(l.p)) continue;
            data[n*4]=b.x; data[n*4+1]=b.y; data[n*4+2]=b.right(); data[n*4+3]=b.bottom(); ++n;
        }
        gl.Uniform1i(uniform(lightProgram,"blockers"),n);
        if (n) gl.Uniform4fv(uniform(lightProgram,"occluders[0]"),n,data.data());
        gl.Uniform1i(uniform(lightProgram,"soft"),high?1:0);
        gl.Uniform4f(uniform(lightProgram,"lightData"),l.p.x,l.p.y,r,l.energy);
        gl.Uniform3f(uniform(lightProgram,"lightColor"),l.tint.r,l.tint.g,l.tint.b);
        gl.Uniform2f(uniform(lightProgram,"direction"),l.direction.x,l.direction.y);
        gl.Uniform1f(uniform(lightProgram,"cone"),l.cone);
        screen(lightProgram);
    }
    gl.Disable(GL_SCISSOR_TEST); gl.Disable(GL_BLEND);
}
void Renderer::post(const Game& g,bool high,bool motionFx) {
    bind(lit,false);
    gl.UseProgram(atmosphereProgram);
    useTexture(atmosphereProgram,"world",0,raw.texture.id);
    useTexture(atmosphereProgram,"lighting",1,lightMap.texture.id);
    const Color tint=hex(g.stage->cold);
    gl.Uniform3f(uniform(atmosphereProgram,"haze"),tint.r,tint.g,tint.b);
    gl.Uniform1f(uniform(atmosphereProgram,"camera"),g.camera);
    gl.Uniform1f(uniform(atmosphereProgram,"time"),g.time);
    gl.Uniform1f(uniform(atmosphereProgram,"fogStrength"),g.index==1 || g.index==2?.72f:.55f);
    gl.Uniform2f(uniform(atmosphereProgram,"lightTexel"),1.f/lightMap.texture.w,1.f/lightMap.texture.h);
    screen(atmosphereProgram);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    actors(g);
    flush();
    gl.Disable(GL_BLEND);

    const int levels=high?4:2;
    Texture from=lit.texture;
    for (int i=0;i<levels;++i) {
        bind(bloom[i],false); gl.UseProgram(downProgram);
        useTexture(downProgram,"source",0,from.id);
        gl.Uniform2f(uniform(downProgram,"texel"),1.f/from.w,1.f/from.h);
        gl.Uniform1i(uniform(downProgram,"first"),i==0?1:0);
        screen(downProgram); from=bloom[i].texture;
    }
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_ONE,GL_ONE);
    for (int i=levels-2;i>=0;--i) {
        bind(bloom[i],false); gl.UseProgram(upProgram);
        useTexture(upProgram,"source",0,from.id);
        gl.Uniform2f(uniform(upProgram,"texel"),1.f/from.w,1.f/from.h);
        screen(upProgram); from=bloom[i].texture;
    }
    gl.Disable(GL_BLEND); output(); gl.UseProgram(presentProgram);
    useTexture(presentProgram,"world",0,lit.texture.id);
    useTexture(presentProgram,"bloom",1,bloom[0].texture.id);
    struct Grade { float shadow[3],highlight[3],saturation; };
    static constexpr Grade grades[]{
        {{.92f,1.00f,1.07f},{1.12f,1.03f,.90f},1.22f}, {{.88f,1.00f,1.05f},{1.00f,1.10f,1.07f},1.24f},
        {{1.03f,.92f,1.09f},{1.06f,1.10f,.94f},1.28f}, {{1.00f,.90f,.88f},{1.16f,1.02f,.86f},1.26f},
        {{.92f,.93f,1.12f},{1.06f,1.02f,1.12f},1.24f}};
    const auto& look=grades[g.index];
    const auto& p=g.player.box;
    gl.Uniform1f(uniform(presentProgram,"strength"),high?.89f:.64f);
    gl.Uniform1f(uniform(presentProgram,"time"),clockTime);
    gl.Uniform1f(uniform(presentProgram,"slow"),clamp((1-g.timeScale)/.72f));
    gl.Uniform1f(uniform(presentProgram,"hurt"),g.pain);
    gl.Uniform1f(uniform(presentProgram,"punch"),motionFx?g.punch:0.f);
    gl.Uniform1f(uniform(presentProgram,"flash"),g.flash);
    gl.Uniform1f(uniform(presentProgram,"combo"),clamp(g.combo/6.f)*(g.comboTimer>0?1.f:0.f));
    gl.Uniform1f(uniform(presentProgram,"saturation"),look.saturation);
    gl.Uniform2f(uniform(presentProgram,"focus"),clamp((p.center().x+offset.x)/width,.2f,.8f),
                 1-clamp((p.center().y+offset.y)/height,.2f,.8f));
    gl.Uniform3f(uniform(presentProgram,"shadowTint"),look.shadow[0],look.shadow[1],look.shadow[2]);
    gl.Uniform3f(uniform(presentProgram,"highlightTint"),look.highlight[0],look.highlight[1],look.highlight[2]);
    screen(presentProgram);
}
} // namespace vl
