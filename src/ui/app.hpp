#pragma once
#include "input/input.hpp"
#include "render/renderer.hpp"
#include "services/services.hpp"
#include "net/world.hpp"
#include "net/pause.hpp"
#include <functional>

namespace vl {
class App : private AudioSink {
public:
    App(SDL_Window* window,Renderer& renderer,Audio& audio,Storage& storage);
    void event(const SDL_Event& event);
    void update(float dt);
    void sampleFrame(float seconds);
    void draw();
    void launch(int sector);
    void sceneForTest(std::string_view scene);
    Input& controls() { return input; }
    // Region around the operative in interface units, or an empty rect. Used by the diagnostics screenshots.
    Rect followCrop(bool enabled) const;
    bool active() const { return running; }
private:
    enum class View { splash,menu,multiplayer,create,load,campaign,armory,settings,controls,credits,intro,coopLoading,play,pause,failed,epilogue,cleared,confirm };
    struct Button { Rect box; std::function<void()> action; };
    SDL_Window* window;
    Renderer& r;
    Audio& audio;
    Storage& store;
    Game game;
    Settings settings;
    Save save;
    View view=View::splash,settingsReturn=View::menu,confirmReturn=View::menu;
    bool running=true,finished=false;
    int rebinding=-1;
    int slot=0,newSlot=1,newCharacter=0,newDifficulty=1,sector=0,weapon=0,focus=-1;
    float clock=0,film=0,accumulator=0,previousCamera=0,notice=0,startup=0;
    Player previousPlayer,previousPartner;
    net::Session network;
    std::string advertisedAddress,joinKey;
    int lanPage=0;
    enum class Lobby {menu,host,join};
    Lobby lobby=Lobby::menu;
    int coopDifficulty=1,coopCharacter=0,remoteCharacter=-1;
    CoopMode coopMode=CoopMode::campaign;
    bool characterLocked=false,remoteLocked=false,rosterPreview=false;
    bool modeAnnounced=false;
    bool openingSkipped=false,skipRequested=false;
    uint32_t mission=0,simulationTick=0,lastSnapshotTick=0;
    net::HostClock hostClock;
    net::Millis preparedAt=0,filmAt=0,deployAt=0,probeAt=0,lastRemoteInput=0,guestLead=0;
    net::Millis epilogueAt=0;
    int probes=0;
    size_t preloadStep=0;
    bool guestLoaded=false,scheduleConfirmed=false,loadingMusic=false;
    WorldMotion previousWorld;
    float netFrame=0,netInterpolation=1;
    bool coopStarted=false,coopPaused=false,snapshot=false;
    net::PauseState sharedPause{};
    bool pauseReplyPending=false;
    float fpsWindowSeconds=0;
    int fpsWindowFrames=0,fpsDisplayed=60;
    Controls pendingControls{};
    std::vector<net::Sound> netSounds;
    void event(std::string_view name,float gain,float pan,float rate) override;
    bool lobbyInput(const SDL_Event& event);
    void multiplayer();
    void openMultiplayer();
    void leaveCoop(const std::string& message={});
    void requestCoopPause();
    void setCoopPause(net::PauseState next);
    void pollCoop(float dt);
    void advanceCoop(float dt);
    void startCoop(int index,bool retry=false);
    void prepareCoop();
    void loadCoop();
    void showCoopOutcome();
    void probeHost();
    void deployCoop();
    void coopCinematic();
    void drawOpening(bool team);
    void skipPrologue();
    void lockOperative();
    void beginEpilogue();
    void updateEpilogue(float dt);
    void skipEpilogue();
    void endEpilogue();
    void sectorCinematic();
    void levelSummary();
    void receiveCoop(std::span<const uint8_t> data);
    void sendWorld();
    Controls readCoopControls() const;
    void coopOverlay();
    Vec mouse{-100,-100};
    CameraTilt cameraTilt;
    bool mouseAiming=false,mouseMoved=false,worldFrame=false;
    void movePointer(Vec position);
    Vec aimPoint() const;
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
    void splash();
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
