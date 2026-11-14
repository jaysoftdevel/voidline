#pragma once
#include "game/game.hpp"
#include "platform/gl.hpp"
#include "platform/sdl.hpp"
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
    // Feeds the measured frame time so the world resolution can follow the available GPU time.
    void adapt(float frameMs);
    float renderScale() const { return resolution; }
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
    // Gradient trapezoid used for light shafts. Colors are given at the narrow and the wide end.
    void beam(Vec from,float fromWidth,Vec to,float toWidth,Color start,Color end);
    // Soft rotating cloud sprite.
    void puff(Vec center,float radius,float angle,Color color);
    // Draws a weapon for menus and previews, fitted inside the box.
    void showcaseGun(int gun,Rect box);
    void outlinedText(Vec baseline,std::string_view value,float size,Color color,Color ink=hex(0x0a0d14));
    // Parallelogram panel. A positive slant leans the top edge to the right.
    void slab(Rect r,float slant,Color top,Color bottom);
    void slabOutline(Rect r,float slant,Color color,float weight=1);
    void text(Vec baseline,std::string_view value,float size,Color color,bool bold=false);
    float measure(std::string_view text,float size,bool bold=false) const;
    Vec mousePosition(int x,int y) const;
    // Saves the frame as a PPM. A non-empty crop (in 1200 x 600 interface units) saves only that region.
    void screenshot(const std::filesystem::path& path,Rect crop={});
    std::string driver() const;
    void check();
    // Drawing toolbox shared by the figure, scene and interface sources.
    void polygon(Vec a,Vec b,Vec c,Vec d,Color color);
    void armor(Vec a,Vec b,float top,float bottom,Color tint,Color edge,bool near);
    void limb(Vec a,Vec b,float thickness,Color plate,Color rim);
    void emissive(Vec at,float radius,Color tint,float energy=1);
    void sprite(std::string_view name,Vec anchor,Vec axisU,Vec axisV,Vec anchorUV,Color tint={});
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
        atmosphereProgram=0,downProgram=0,upProgram=0,presentProgram=0,materialProgram=0,fxaaProgram=0;
    Texture white,radial,cloud;
    std::array<Font,2> fonts{};
    struct NameHash { using is_transparent=void; size_t operator()(std::string_view v) const { return std::hash<std::string_view>{}(v); } };
    std::unordered_map<std::string,Texture,NameHash,std::equal_to<>> images;
    std::vector<Vertex> vertices;
    GLuint batchTexture=0;
    int pixelW=0,pixelH=0,viewX=0,viewY=0,viewW=0,viewH=0;
    GLint materialCamera=-1,materialSector=-1;
    Target raw,mirror,lightMap,lit,ldr;
    // Actors are tinted by the lights near them so they sit in the scene instead of floating over it.
    Color actorLight{1,1,1,1};
    bool tinting=false;
    Color shade(Color c) const { return tinting?Color{c.r*actorLight.r,c.g*actorLight.g,c.b*actorLight.b,c.a}:c; }
    Color litBy(Vec p,const Game& game) const;
    std::array<Target,4> bloom{};
    Vec offset,playerShift;
    // Optional rotation applied to every vertex while drawing a spinning actor.
    bool spinning=false;
    Vec spinPivot;
    float spinCos=1,spinSin=0;
    Vec transform(Vec p) const;
    void beginSpin(Vec pivot,float angle);
    void endSpin();
    Gait pose;
    float carry=0,clockTime=0,resolution=1,smoothedMs=8;
    int slowFrames=0,fastFrames=0;
    std::array<Vec,2> drawHeels{};
    std::array<float,2> drawFootAngles{};
    std::vector<Light> lights;
    std::vector<Rect> blockers;
    GLuint program(const char* vertex,const char* fragment);
    Texture texture(int w,int h,const void* pixels,GLenum format=GL_RGBA8);
    Texture readTexture(const std::filesystem::path& path);
    Texture& asset(std::string_view name);
    Target target(int w,int h,GLenum format=GL_R11F_G11F_B10F);
    void destroy(Target& target);
    void bind(const Target& target,bool clear);
    void output();
    void flush();
    void quad(Rect r,Color top,Color bottom,GLuint id,Rect uv={0,0,1,1});
    void useTexture(GLuint shader,const char* uniform,int slot,GLuint id);
    // Uniform names are string literals, so their addresses identify them and the lookup is done once per program.
    GLint uniform(GLuint shader,const char* name) {
        const uint64_t key=(uint64_t(shader)<<48)^reinterpret_cast<uintptr_t>(name);
        const auto it=uniforms.find(key);
        if (it!=uniforms.end()) return it->second;
        return uniforms.emplace(key,gl.GetUniformLocation(shader,name)).first->second;
    }
    std::unordered_map<uint64_t,GLint> uniforms;
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
    void drawEnemy(const Enemy& enemy,Color accent,int sector,float time,Vec target);
    // Enemy drawing (render/figures/enemy_*.cpp), one function per type.
    struct EnemyView;
    void drawDrone(const EnemyView& view);
    void drawTurret(const EnemyView& view);
    void drawStalker(const EnemyView& view);
    void drawSoldier(const EnemyView& view);
    void drawGuardian(const EnemyView& view);
    void drawWounds(const EnemyView& view);
    void drawEnemyBar(const EnemyView& view);
    void drawCorpses(const Game& game);
    // Human figures (render/figures). The rig is built once per figure and handed to each part.
    struct Humanoid; struct Rig; struct ArmPose; struct Face;
    void humanoid(const Humanoid& figure);
    Rig buildRig(const Humanoid& figure) const;
    Vec spun(const Humanoid& figure,Vec point) const;
    void drawLegs(const Humanoid& figure,const Rig& rig);
    void drawScarf(const Humanoid& figure,const Rig& rig);
    void drawPack(const Humanoid& figure,const Rig& rig);
    void drawTorso(const Humanoid& figure,const Rig& rig);
    Face drawHead(const Humanoid& figure,const Rig& rig);
    void drawVossHead(const Humanoid& figure,const Face& face);
    void drawRookHead(const Humanoid& figure,const Face& face);
    void drawValeHead(const Humanoid& figure,const Face& face);
    void drawHelmet(const Humanoid& figure,const Face& face);
    void drawArms(const Humanoid& figure,const Rig& rig);
    static ArmPose blendArms(const ArmPose& a,const ArmPose& b,float t);
    ArmPose freeArm(const Humanoid& figure,const Rig& rig,Vec shoulder,bool near) const;
    ArmPose gripArm(const Humanoid& figure,Vec shoulder,Vec hand,bool near) const;
    void drawArm(const Humanoid& figure,const ArmPose& arm,bool near,bool gripping);
    void drawHand(const Humanoid& figure,const ArmPose& arm,bool near,bool gripping);
    void drawParticles(const Game& game);
    void drawRings(const Game& game);
    void ambient(const Game& game);
    void shafts(const Game& game);
    void gunModel(int gun,Vec muzzle,Vec aim,Vec up,float scale,Color glow);
    void drawTrails(const Game& game,Vec ground);
    void collectLights(const Game& game);
    void drawLights(bool high);
    void water(const Game& game,bool high);
    void post(const Game& game,bool high,bool motionFx);
    bool visible(float x,float w=0) const { return x+offset.x+w>=-120 && x+offset.x<=width+120; }
};
}
