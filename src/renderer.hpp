#pragma once
#include "game.hpp"
#include "gl.hpp"
#include "platform.hpp"
#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace vl {
class Renderer {
public:
    Renderer(SDL_Window* window,std::filesystem::path assets);
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    void begin();
    void end();
    void drawWorld(const Game& game,bool high,bool motion,float interpolation,const Player& previous,float previousCamera);
    void cover(std::string_view name,Rect box,float zoom=1,Vec bias={.5f,.5f},Color tint={});
    void image(std::string_view name,Rect box,Color tint={});
    void rect(Rect r,Color color);
    void gradient(Rect r,Color top,Color bottom);
    void outline(Rect r,Color color,float weight=1);
    void line(Vec a,Vec b,float weight,Color color);
    void ellipse(Vec center,Vec radius,Color color);
    void halo(Vec center,float radius,Color color);
    void triangle(Vec a,Vec b,Vec c,Color color);
    void outlinedText(Vec baseline,std::string_view value,float size,Color color,Color ink=hex(0x0a0d14));
    // Parallelogram panel. A positive slant leans the top edge to the right.
    void slab(Rect r,float slant,Color top,Color bottom);
    void slabOutline(Rect r,float slant,Color color,float weight=1);
    void text(Vec baseline,std::string_view value,float size,Color color,bool bold=false);
    float measure(std::string_view text,float size,bool bold=false) const;
    Vec mousePosition(int x,int y) const;
    void screenshot(const std::filesystem::path& path);
    std::string driver() const;
    void check();
private:
    struct Vertex { Vec p,uv; Color tint; };
    struct Texture { GLuint id=0; int w=0,h=0; };
    struct Target { Texture texture; GLuint framebuffer=0; };
    struct Font { Texture texture; std::array<float,95> advances{}; };
    struct Light {
        Vec p; float radius,energy;
        Color tint; Vec direction={0,1}; float cone=-1;
        bool casts=true;
    };
    SDL_Window* window;
    std::filesystem::path assets;
    GL gl;
    GLuint vao=0,buffer=0,drawProgram=0,lightProgram=0,waterProgram=0,
        atmosphereProgram=0,downProgram=0,upProgram=0,presentProgram=0,materialProgram=0;
    Texture white,radial;
    std::array<Font,2> fonts{};
    std::unordered_map<std::string,Texture> images;
    std::vector<Vertex> vertices;
    GLuint batchTexture=0;
    int pixelW=0,pixelH=0,viewX=0,viewY=0,viewW=0,viewH=0;
    GLint materialCamera=-1,materialSector=-1;
    Target raw,mirror,lightMap,lit;
    std::array<Target,4> bloom{};
    Vec offset,playerShift;
    // Optional rotation applied to every vertex while drawing a spinning actor.
    bool spinning=false;
    Vec spinPivot;
    float spinCos=1,spinSin=0;
    Vec transform(Vec p) const;
    void beginSpin(Vec pivot,float angle);
    void endSpin();
    void sprite(std::string_view name,Vec anchor,Vec axisU,Vec axisV,Vec anchorUV,Color tint={});
    Gait pose;
    float carry=0,clockTime=0;
    std::array<Vec,2> drawHeels{};
    std::array<float,2> drawFootAngles{};
    std::vector<Light> lights;
    std::vector<Rect> blockers;
    GLuint program(const char* vertex,const char* fragment);
    Texture texture(int w,int h,const void* pixels,GLenum format=GL_RGBA8);
    Texture readTexture(const std::filesystem::path& path);
    Texture& asset(std::string_view name);
    Target target(int w,int h);
    void destroy(Target& target);
    void bind(const Target& target,bool clear);
    void output();
    void flush();
    void quad(Rect r,Color top,Color bottom,GLuint id,Rect uv={0,0,1,1});
    void useTexture(GLuint shader,const char* uniform,int slot,GLuint id);
    GLint uniform(GLuint shader,const char* name) { return gl.GetUniformLocation(shader,name); }
    void screen(GLuint shader);
    void backdrop(const Game& game);
    void terrain(const Game& game);
    void material(const Game& game);
    void depthScenery(const Game& game);
    void foreground(const Game& game);
    void scenery(const Game& game);
    void objects(const Game& game);
    void actors(const Game& game);
    void drawPlayer(const Game& game);
    void armor(Vec a,Vec b,float top,float bottom,Color tint,Color edge,bool near);
    void drawEnemy(const Enemy& enemy,Color accent,int sector,float time,Vec target);
    void drawCorpses(const Game& game);
    struct Humanoid;
    void humanoid(const Humanoid& figure);
    void polygon(Vec a,Vec b,Vec c,Vec d,Color color);
    void drawParticles(const Game& game);
    void drawRings(const Game& game);
    void drawTrails(const Game& game,Vec ground);
    void collectLights(const Game& game);
    void drawLights(bool high);
    void water(const Game& game,bool high);
    void post(const Game& game,bool high,bool motionFx);
    void emissive(Vec at,float radius,Color tint,float energy=1);
    void limb(Vec a,Vec b,float thickness,Color plate,Color rim);
    bool visible(float x,float w=0) const { return x+offset.x+w>=-120 && x+offset.x<=width+120; }
};
}
