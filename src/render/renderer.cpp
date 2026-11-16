#include "render/renderer.hpp"
#include "render/fps_border.hpp"
#include "render/shaders.hpp"
#include "render/shaders/cinematic.hpp"
#include <chrono>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace vl {
Renderer::Renderer(SDL_Window* win,std::filesystem::path art,bool softwareMode)
    :window(win),assets(art),software(softwareMode) {
    gl.load();
    hdrFormat=chooseHdrFormat();
    drawProgram=program(shader::mesh,shader::texture);
    materialProgram=program(shader::mesh,shader::material);
    materialCamera=uniform(materialProgram,"camera");
    materialSector=uniform(materialProgram,"sector");
    materialDetail=uniform(materialProgram,"detail");
    lightProgram=program(shader::fullscreen,shader::light);
    waterProgram=program(shader::fullscreen,shader::water);
    atmosphereProgram=program(shader::fullscreen,shader::atmosphere);
    downProgram=program(shader::fullscreen,shader::down);
    upProgram=program(shader::fullscreen,shader::up);
    presentProgram=program(shader::fullscreen,shader::present);
    fxaaProgram=program(shader::fullscreen,shader::fxaa);
    cinematicProgram=program(shader::fullscreen,shader::cinematic);
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
    {
        constexpr int size=96;
        std::vector<uint8_t> pixels(size*size*4,255);
        for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
            const float across=2.f*x/(size-1)-1.f, along=float(y)/(size-1);
            const float taper=smooth(1.f-std::abs(across));
            pixels[(y*size+x)*4+3]=uint8_t(std::lround(255*taper*(1-along)*(1-along)));
        }
        shaftTexture=texture(size,size,pixels.data());
    }
    // A tileless cloud: five octaves of value noise under a soft radial mask.
    {
        constexpr int size=128;
        std::vector<uint8_t> pixels(size*size*4,255);
        auto lattice=[](int x,int y) {
            uint32_t n=uint32_t(x)*374761393u+uint32_t(y)*668265263u;
            n=(n^(n>>13))*1274126177u;
            return float((n^(n>>16))&0xffff)/65535.f;
        };
        auto noise=[&](float x,float y) {
            const int ix=int(std::floor(x)),iy=int(std::floor(y));
            const float fx=smooth(x-ix),fy=smooth(y-iy);
            return mix(mix(lattice(ix,iy),lattice(ix+1,iy),fx),mix(lattice(ix,iy+1),lattice(ix+1,iy+1),fx),fy);
        };
        for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
            const float u=(x+.5f)/size*2-1,v=(y+.5f)/size*2-1;
            const float d=std::hypot(u,v);
            float f=0,amp=.5f,freq=3.f;
            for (int octave=0;octave<5;++octave) { f+=amp*noise(x/float(size)*freq+7.f*octave,y/float(size)*freq+3.f*octave); amp*=.5f; freq*=2.f; }
            const float mask=smooth(clamp((1-d)/.65f));
            pixels[(y*size+x)*4+3]=uint8_t(std::lround(clamp(mask*(f*1.55f-.22f))*255));
        }
        cloud=texture(size,size,pixels.data());
    }
    for (size_t i=0;i<fonts.size();++i) {
        const std::string base=i==0?"fonts/font-regular":"fonts/font-black";
        fonts[i].texture=readTexture(base+".png");
        const auto metrics=assets.read(base+".txt");
        std::istringstream in(std::string(metrics.begin(),metrics.end()));
        for (auto& v:fonts[i].advances) in>>v;
        if (!in) throw std::runtime_error("Invalid packaged font metrics: "+base);
    }
    raw=target(1200,600); mirror=target(600,300); lightMap=target(480,240); lit=target(1200,600); ldr=target(1200,600,GL_RGBA8);
    int w=600,h=300;
    for (auto& level:bloom) { level=target(w,h); w/=2; h/=2; }
    vertices.reserve(28000); lights.reserve(64); blockers.reserve(50);
    check();
}

Renderer::~Renderer() {
    for (auto& t:bloom) destroy(t);
    destroy(raw); destroy(mirror); destroy(lightMap); destroy(lit); destroy(ldr);
    for (auto& frame:filmFrames) if (frame.framebuffer) destroy(frame);
    gl.DeleteTextures(1,&white.id); gl.DeleteTextures(1,&radial.id); gl.DeleteTextures(1,&cloud.id);
    gl.DeleteTextures(1,&shaftTexture.id);
    for (auto& font:fonts) gl.DeleteTextures(1,&font.texture.id);
    for (auto& [name,t]:images) gl.DeleteTextures(1,&t.id);
    for (GLuint id:{drawProgram,lightProgram,waterProgram,atmosphereProgram,downProgram,upProgram,presentProgram,materialProgram,fxaaProgram}) gl.DeleteProgram(id);
    gl.DeleteProgram(cinematicProgram);
    gl.DeleteBuffers(1,&buffer); gl.DeleteVertexArrays(1,&vao);
}

void Renderer::setGraphics(const GraphicsQuality& value) {
    graphics=value;
    if (!graphics.dynamicResolution) {
        resolution=1.f; slowFrames=fastFrames=0;
    }
}

void Renderer::adapt(float frameMs) {
    if (!graphics.dynamicResolution) return;
    smoothedMs=mix(smoothedMs,std::min(frameMs,60.f),.05f);
    // Step down quickly when frames miss a 50 fps budget and back up slowly once there is clear headroom.
    slowFrames=smoothedMs>20.5f?slowFrames+1:0;
    fastFrames=smoothedMs<10.5f?fastFrames+1:0;
    if (slowFrames>20 && resolution>.64f) { resolution=resolution>.8f?.75f:.625f; slowFrames=0; smoothedMs=16; }
    else if (fastFrames>360 && resolution<1.f) { resolution=resolution<.7f?.75f:1.f; fastFrames=0; }
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

bool Renderer::drawFpsOutside(int fps,int corner) {
    // Keep the font at its normal physical size: temporarily shift the same
    // 1200x600 OpenGL viewport so only this badge lands in the black gutter.
    const int panelW=std::max(1,int(std::lround(93.f*viewW/width)));
    const int panelH=std::max(1,int(std::lround(27.f*viewH/height)));
    const int margin=std::max(4,int(std::lround(6.f*viewH/height)));
    const auto badge=fpsBorderPlacement(pixelW,pixelH,viewX,viewY,viewW,viewH,
                                        corner,panelW,panelH,margin);
    if (!badge.outside) return false;
    flush();
    gl.Viewport(badge.x,pixelH-badge.y-viewH,viewW,viewH);
    const Rect tile{0,0,93,27};
    rect(tile,hex(0x06121b).alpha(.85f));
    outline(tile,hex(0x3a8794).alpha(.75f));
    text({10,19},std::to_string(fps)+" FPS",15,hex(0x8ef4da),true);
    output(); // Flush the badge and restore the main content viewport.
    return true;
}

void Renderer::end() { flush(); SDL_GL_SwapWindow(window); }

void Renderer::check() {
    if (const auto error=gl.GetError()) throw std::runtime_error("OpenGL error "+std::to_string(error));
}

std::string Renderer::driver() const {
    return std::string(reinterpret_cast<const char*>(gl.GetString(GL_RENDERER)))+" / "+reinterpret_cast<const char*>(gl.GetString(GL_VERSION));
}

Vec Renderer::mousePosition(int x,int y) const {
    int w=1,h=1; SDL_GetWindowSize(window,&w,&h);
    return {(x*float(pixelW)/std::max(1,w)-viewX)*width/std::max(1,viewW),
            (y*float(pixelH)/std::max(1,h)-(pixelH-viewY-viewH))*height/std::max(1,viewH)};
}

void Renderer::screenshot(const std::filesystem::path& file,Rect crop) {
    flush();
    std::vector<uint8_t> bytes(size_t(pixelW)*pixelH*4);
    gl.ReadPixels(0,0,pixelW,pixelH,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());
    int x0=0,y0=0,x1=pixelW,y1=pixelH;
    if (crop.w>0 && crop.h>0) {
        x0=std::clamp(int(viewX+crop.x/width*viewW),0,pixelW-1); x1=std::clamp(int(viewX+crop.right()/width*viewW),x0+1,pixelW);
        y0=std::clamp(int(viewY+crop.y/height*viewH),0,pixelH-1); y1=std::clamp(int(viewY+crop.bottom()/height*viewH),y0+1,pixelH);
    }
    std::ofstream out(file,std::ios::binary);
    out<<"P6\n"<<(x1-x0)<<" "<<(y1-y0)<<"\n255\n";
    for (int y=y0;y<y1;++y) for (int x=x0;x<x1;++x) out.write(reinterpret_cast<const char*>(&bytes[((pixelH-1-y)*pixelW+x)*4]),3);
    if (!out) throw std::runtime_error("Could not save screenshot "+file.string());
}
} // namespace vl
