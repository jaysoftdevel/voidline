#pragma once
#include "input/input.hpp"
#include "render/renderer.hpp"
#include "services/services.hpp"
#include <functional>

namespace vl {
class App {
public:
    App(SDL_Window* window,Renderer& renderer,Audio& audio,Storage& storage);
    void event(const SDL_Event& event);
    void update(float dt);
    void draw();
    void launch(int sector);
    void sceneForTest(std::string_view scene);
    Input& controls() { return input; }
    // Region around the operative in interface units, or an empty rect. Used by the diagnostics screenshots.
    Rect followCrop(bool enabled) const;
    bool active() const { return running; }
private:
    enum class View { menu,create,load,campaign,armory,settings,controls,credits,intro,play,pause,failed,cleared,confirm };
    struct Button { Rect box; std::function<void()> action; };
    SDL_Window* window;
    Renderer& r;
    Audio& audio;
    Storage& store;
    Game game;
    Settings settings;
    Save save;
    View view=View::menu,settingsReturn=View::menu,confirmReturn=View::menu;
    bool running=true,finished=false;
    int rebinding=-1;
    int slot=0,newSlot=1,newCharacter=0,newDifficulty=1,sector=0,weapon=0,focus=-1;
    float clock=0,film=0,accumulator=0,previousCamera=0,notice=0;
    Player previousPlayer;
    Vec mouse{-100,-100};
    std::array<std::optional<Save>,3> slots;
    std::vector<Button> buttons;
    std::string notification,confirmTitle,confirmCopy;
    std::function<void()> confirmAction;
    Input input;
    std::function<void(int)> script;
    int scriptFrame=0;
    bool followEnemy=false;
    void show(View v);
    void closeAction();
    void driveGame(float dt);
    void cycleWeapon(int direction);
    void navigate(int direction);
    void activate();
    void changeFullscreen();
    void commitSettings();
    bool persist(Save& next,int target);
    void inform(std::string message);
    void refresh();
    void newGame();
    void beginCampaign();
    void loadCampaign(int from);
    void finishMission();
    void showMap();
    void showArmory();
    void retry();
    void buy(int rank);
    void buyPerk(int index);
    int upgradePrice(int rank) const;
    void equip();
    void ask(View previous,std::string title,std::string text,std::function<void()> action);
    void button(Rect box,std::string label,std::function<void()> action,bool enabled=true,bool primary=false);
    void hit(Rect box,std::function<void()> action,bool enabled);
    bool hovered(Rect box) const;
    void label(Vec at,std::string_view text,Color tint=hex(0x94aabc));
    void copy(Vec at,std::string_view text,float maxWidth,float size=16,Color tint=hex(0xd1dbe4));
    void shell(std::string_view art);
    void heading(std::string_view tag,std::string_view title,std::string_view detail={});
    void menu();
    void create();
    void load();
    void campaign();
    void armory();
    void options();
    void keyBindings();
    void diagnostics();
    void captureKey(int scancode);
    void credits();
    void cinematic();
    void hud();
    void overlay();
};
}
