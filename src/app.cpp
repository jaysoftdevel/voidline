#include "app.hpp"
#include <iomanip>
#include <limits>
#include <sstream>

namespace vl {
namespace {
const Color amber=hex(0xffac71), white=hex(0xf2f5fa), pale=hex(0xbbc8d4), muted=hex(0x8397a9), teal=hex(0x8ef4da);
std::string digits(int n) { return n<10?"0"+std::to_string(n):std::to_string(n); }
std::string timeText(float secs) {
    const int total=std::max(0,int(secs));
    return std::to_string(total/60)+":"+digits(total%60);
}
std::string sectorTrack(int index) { return "sector-"+std::to_string(index+1); }
int total(int current,int earned) { return current+std::min(earned,std::numeric_limits<int>::max()-current); }
constexpr Rect screen{0,0,width,height};
}
App::App(SDL_Window* win,Renderer& renderer,Audio& mixer,Storage& storage)
    :window(win),r(renderer),audio(mixer),store(storage),game(audio) {
    settings=store.settings();
    audio.configure(settings);
    audio.track("menu");
    refresh();
}
void App::refresh() { for (int i=0;i<3;++i) slots[i]=store.get(i+1); }
void App::show(View v) {
    view=v; focus=-1; buttons.clear();
    SDL_ShowCursor(v==View::play?0:1);
    if (v!=View::play && v!=View::pause && v!=View::intro && v!=View::failed) audio.track("menu");
}
void App::inform(std::string message) { notification=std::move(message); notice=4; }
bool App::persist(Save& next,int target) {
    try {
        store.put(target,next); slots[target-1]=next; return true;
    } catch (const std::exception& e) {
        inform(e.what()); return false;
    }
}
void App::commitSettings() {
    audio.configure(settings);
    try { store.put(settings); }
    catch (const std::exception& e) { inform(e.what()); }
}
void App::changeFullscreen() {
    const U32 flags=(SDL_GetWindowFlags(window)&sdl::fullscreen)?0:sdl::fullscreen;
    if (SDL_SetWindowFullscreen(window,flags)<0) inform(SDL_GetError());
}
void App::ask(View previous,std::string title,std::string text,std::function<void()> action) {
    confirmReturn=previous; confirmTitle=std::move(title); confirmCopy=std::move(text); confirmAction=std::move(action);
    show(View::confirm);
}
void App::closeAction() {
    switch (view) {
    case View::menu: running=false; break;
    case View::settings: show(settingsReturn); break;
    case View::intro: launch(0); break;
    case View::play: game.pause(true); accumulator=0; audio.track(""); show(View::pause); break;
    case View::pause: game.pause(false); audio.track(sectorTrack(game.index)); show(View::play); break;
    case View::armory:
    case View::cleared:
    case View::failed: showMap(); break;
    case View::confirm: show(confirmReturn); break;
    default: show(View::menu); break;
    }
}
void App::navigate(int direction) {
    const int count=static_cast<int>(buttons.size());
    if (count==0) return;
    if (direction>0) focus=(focus+1)%count;
    else focus=focus<0?count-1:(focus+count-1)%count;
}
void App::activate() {
    const int count=static_cast<int>(buttons.size());
    if (count==0) return;
    auto action=buttons[std::clamp(focus,0,count-1)].action; action();
}
void App::event(const SDL_Event& e) {
    input.event(e);
    if (e.type==sdl::quit) { running=false; return; }
    if (e.type==sdl::window && e.window.event==sdl::focusLost) {
        if (view==View::play) closeAction();
        return;
    }
    if (e.type==sdl::mouseMotion) {
        mouse=r.mousePosition(e.motion.x,e.motion.y); focus=-1;
        if (view==View::play) game.pointer(mouse);
    } else if (e.type==sdl::mouseDown && e.button.button==1) {
        mouse=r.mousePosition(e.button.x,e.button.y);
        if (view==View::play) game.pointer(mouse);
        else for (const auto& b:buttons) if (b.box.contains(mouse)) { auto action=b.action; action(); break; }
    }
    if (e.type==sdl::controllerButton && e.cbutton.state && view!=View::play) {
        using namespace sdl::pad;
        const int button=e.cbutton.button;
        if (button==down || button==right) navigate(1);
        else if (button==up || button==left) navigate(-1);
        else if (button==a) activate();
        else if (button==b && view!=View::pause) closeAction();
        return;
    }
    if (e.type!=sdl::keyDown || e.key.repeat) return;
    const int key=e.key.keysym.scancode;
    if (key==sdl::key::f11) { changeFullscreen(); return; }
    // Gameplay input is polled each frame. Pause is toggled by the polled pause action.
    if (view==View::play) return;
    if (key==sdl::key::esc) { if (view!=View::pause) closeAction(); return; }
    if (view==View::intro && (key==sdl::key::space || key==sdl::key::enter)) { launch(0); return; }
    if (key==sdl::key::down || key==sdl::key::right || key==sdl::key::tab) navigate(1);
    else if (key==sdl::key::up || key==sdl::key::left) navigate(-1);
    else if (key==sdl::key::enter || key==sdl::key::space) activate();
}
void App::cycleWeapon(int direction) {
    for (int step=1;step<=4;++step) {
        const int index=((game.gun+direction*step)%4+4)%4;
        if (game.guns[index].available) { game.select(index); return; }
    }
}
// Translates polled input into game state. Every action is evaluated each frame so any chord works.
void App::driveGame(float dt) {
    auto apply=[&](Key key,Act act) {
        if (input.pressed(act)) game.input(key,true);
        game.hold(key,input.down(act));
    };
    apply(Key::left,Act::left); apply(Key::right,Act::right); apply(Key::jump,Act::jump);
    apply(Key::fire,Act::fire); apply(Key::automatic,Act::assist); apply(Key::sprint,Act::sprint);
    apply(Key::crouch,Act::crouch); apply(Key::slow,Act::slow);
    if (input.pressed(Act::reload)) game.input(Key::reload,true);
    game.hold(Key::reload,false);
    for (int i=0;i<4;++i) if (input.pressed(static_cast<Act>(static_cast<int>(Act::weapon1)+i))) game.select(i);
    if (input.pressed(Act::next)) cycleWeapon(1);
    if (input.pressed(Act::previous)) cycleWeapon(-1);
    game.stick(input.aimStick(),input.aimStickActive());
    (void)dt;
}
void App::newGame() {
    refresh(); newCharacter=0; newDifficulty=settings.difficulty; newSlot=1;
    for (int i=0;i<3;++i) if (!slots[i]) { newSlot=i+1; break; }
    show(View::create);
}
void App::beginCampaign() {
    Save next;
    next.character=newCharacter; next.difficulty=newDifficulty;
    if (!persist(next,newSlot)) { show(View::create); return; }
    save=next; slot=newSlot; sector=0;
    if (settings.cinematic) {
        film=0; audio.track("intro",false); show(View::intro);
    } else launch(0);
}
void App::loadCampaign(int target) {
    if (!slots[target-1]) return;
    save=*slots[target-1]; slot=target; sector=std::min(save.cleared,4);
    showMap();
}
void App::launch(int index) {
    sector=index;
    game.start(sector,save);
    previousPlayer=game.player; previousCamera=game.camera;
    accumulator=0; finished=false;
    input.reset();
    audio.track(sectorTrack(index)); audio.event("launch",.65f);
    show(View::play);
}
void App::retry() {
    game.retry();
    previousPlayer=game.player; previousCamera=game.camera;
    accumulator=0; finished=false; input.reset();
    audio.track(sectorTrack(game.index)); show(View::play);
}
void App::showMap() { show(View::campaign); }
void App::showArmory() { weapon=save.equipped; show(View::armory); }
void App::buy(int rank) {
    const int tier=save.tiers[weapon][rank];
    const int price=upgrades[rank].base+tier*22;
    if (save.cleared<weapons[weapon].unlock || tier>=3 || save.credits<price) return;
    auto next=save; ++next.tiers[weapon][rank]; next.credits-=price;
    if (!persist(next,slot)) return;
    save=next; audio.event("upgrade");
    inform(std::string(upgrades[rank].name)+" upgraded to rank "+std::to_string(tier+1)+".");
}
void App::equip() {
    if (save.cleared<weapons[weapon].unlock || weapon==save.equipped) return;
    auto next=save; next.equipped=weapon;
    if (persist(next,slot)) { save=next; audio.event("switch"); }
}
void App::finishMission() {
    if (finished) return;
    finished=true;
    accumulator=0;
    if (game.mode==Mode::failed) {
        audio.track(""); audio.event("failure"); show(View::failed); return;
    }
    if (slot) {
        auto next=save;
        next.cleared=std::max(next.cleared,game.index+1);
        next.score=total(next.score,game.score); next.credits=total(next.credits,game.score);
        auto& record=next.records[game.index];
        if (!record.time || game.score>record.score || (game.score==record.score && game.time<record.time))
            record={game.time,game.score};
        if (persist(next,slot)) save=next;
    }
    sector=std::min(game.index+1,4);
    audio.event("complete"); show(View::cleared);
}
void App::update(float dt) {
    clock+=dt; notice=std::max(0.f,notice-dt);
    if (script) script(scriptFrame++);
    input.poll();
    if (view==View::intro) {
        film+=dt;
        if (film>=15.35f) launch(0);
    }
    if ((view==View::play || view==View::pause) && input.pressed(Act::pause)) closeAction();
    if (view!=View::play) { audio.pace(1); return; }
    driveGame(dt);
    const float scale=game.advance(dt);
    audio.pace(game.timeScale);
    accumulator+=dt*scale;
    while (accumulator>=step && game.mode==Mode::active) {
        previousPlayer=game.player;
        previousCamera=game.camera;
        game.tick();
        accumulator-=step;
    }
    if (game.mode==Mode::complete || game.mode==Mode::failed) finishMission();
}
bool App::hovered(Rect box) const {
    return box.contains(mouse) || (focus>=0 && static_cast<int>(buttons.size())==focus);
}
void App::hit(Rect box,std::function<void()> action,bool enabled) {
    if (enabled) buttons.push_back({box,std::move(action)});
}
void App::button(Rect b,std::string text,std::function<void()> action,bool enabled,bool primary) {
    const bool hover=enabled && hovered(b);
    const Color color=!enabled?hex(0x202a35):primary?(hover?hex(0xffba87):amber):(hover?hex(0x304550):hex(0x172831));
    r.gradient(b,color,color.energy(.85f));
    r.outline(b,hover||primary?amber.alpha(enabled?.89f:.3f):hex(0x3b5262),1.1f);
    const Color ink=!enabled?muted:primary?hex(0x181318):white;
    r.text({b.x+14,b.y+b.h*.5f+5},text,15,ink,true);
    hit(b,std::move(action),enabled);
}
void App::label(Vec at,std::string_view text,Color tint) { r.text(at,text,12,tint,true); }
void App::copy(Vec at,std::string_view text,float maxWidth,float size,Color tint) {
    std::istringstream words{std::string(text)};
    std::string row,word;
    while (words>>word) {
        const std::string combined=row.empty()?word:row+" "+word;
        if (!row.empty() && r.measure(combined,size)>maxWidth) {
            r.text(at,row,size,tint); at.y+=size*1.38f; row=word;
        } else row=combined;
    }
    if (!row.empty()) r.text(at,row,size,tint);
}
void App::shell(std::string_view art) {
    r.cover(art,screen,1.07f,{.58f,.51f},hex(0xc2cfe0));
    r.gradient(screen,hex(0x07111b).alpha(.73f),hex(0x09131e).alpha(.95f));
    r.rect({0,0,width,43},hex(0x0c1721).alpha(.97f));
    r.line({0,43},{width,43},1,hex(0x54616e).alpha(.60f));
    r.image("voidline-logo.png",{37,5,32,32});
    r.text({77,28},"VOIDLINE",18,white,true);
    label({1036,27},"FIELD OPERATIONS",muted);
}
void App::heading(std::string_view tag,std::string_view title,std::string_view detail) {
    label({55,78},tag,amber);
    r.text({53,123},title,48,white,true);
    if (!detail.empty()) r.text({57,151},detail,16,pale);
}
void App::menu() {
    r.cover("cover",screen,1.035f,{.62f,.52f},hex(0xd6e4f3));
    r.gradient(screen,hex(0x0e1722).alpha(.25f),hex(0x090f18).alpha(.83f));
    r.gradient({0,0,568,height},hex(0x070f18).alpha(.95f),hex(0x080f19).alpha(.88f));
    r.rect({0,0,6,height},hex(0xe49a64));
    r.line({563,0},{563,height},1.1f,amber.alpha(.31f));
    r.image("voidline-logo.png",{51,42,70,70});
    label({135,77},"VEGA-9  /  COLONY RESCUE",amber);
    r.text({49,179},"VOIDLINE",96,white,true);
    r.rect({54,204,65,3},amber);
    copy({54,241},"A dead colony. One operative. Fight your way to the orbital uplink.",360,20,pale);
    button({54,310,339,42},"NEW GAME",[this]{newGame();},true,true);
    button({54,360,339,42},"LOAD GAME",[this]{refresh(); show(View::load);});
    button({54,410,339,42},"SETTINGS",[this]{settingsReturn=View::menu; show(View::settings);});
    button({54,460,339,42},"CREDITS",[this]{show(View::credits);});
    button({54,510,339,42},"QUIT",[this]{running=false;});
    label({1003,566},"NATIVE EDITION  /  2.0.0",pale);
}
void App::create() {
    shell("cover");
    heading("NEW CAMPAIGN","Select an operative","Choose a suit, difficulty and save slot.");
    for (int i=0;i<3;++i) {
        const auto& c=characters[i];
        const Rect card{54.f+i*367,177,355,263};
        const bool chosen=i==newCharacter;
        r.cover(c.art,card,1,{.51f,.25f});
        r.gradient(card,hex(0x0b151c).alpha(.04f),hex(0x080e16).alpha(.94f));
        r.rect({card.x,card.bottom()-85,card.w,85},hex(0x101a23).alpha(.75f));
        r.outline(card,chosen?hex(c.color):hovered(card)?white:hex(0x4c5e6a),chosen?2.4f:1.25f);
        label({card.x+13,card.bottom()-60},c.role,hex(c.color));
        r.text({card.x+13,card.bottom()-35},c.name,24,white,true);
        r.text({card.x+13,card.bottom()-13},c.trait,14,pale);
        hit(card,[this,i]{newCharacter=i; audio.event("switch",.6f);},true);
    }
    label({54,463},"DIFFICULTY");
    label({493,463},"SAVE SLOT");
    for (int i=0;i<3;++i) {
        button({54.f+i*135,472,125,36},difficulties[i].name,[this,i]{newDifficulty=i;},true,i==newDifficulty);
        button({493.f+i*101,472,94,36},"Slot "+std::to_string(i+1)+(slots[i]?" *":""),[this,i]{newSlot=i+1;},true,newSlot==i+1);
    }
    button({54,532,119,38},"BACK",[this]{show(View::menu);});
    button({886,532,260,38},"START CAMPAIGN",[this] {
        if (!slots[newSlot-1]) beginCampaign();
        else ask(View::create,"Replace slot "+std::to_string(newSlot)+"?",
                 "Starting a new campaign will replace the progress in this slot.",[this]{beginCampaign();});
    },true,true);
}
void App::load() {
    shell("orbital"); heading("SAVED CAMPAIGNS","Load game","Select a campaign to return to the sector map.");
    for (int i=0;i<3;++i) {
        const Rect box{55,178.f+i*107,1090,92};
        r.gradient(box,hex(0x172a36),hex(0x111e29));
        r.outline(box,hex(0x465a67));
        label({72,box.y+22},"SLOT "+digits(i+1),amber);
        if (slots[i]) {
            const auto& s=*slots[i];
            r.text({71,box.y+52},characters[s.character].name,27,white,true);
            r.text({72,box.y+74},std::string(difficulties[s.difficulty].name)+"    |    "+
                std::to_string(s.cleared)+"/5 sectors    |    "+std::to_string(s.score)+" total score",14,pale);
        } else r.text({72,box.y+59},"Empty slot",22,muted,true);
        button({948,box.y+27,180,39},"LOAD",[this,i]{loadCampaign(i+1);},slots[i].has_value(),true);
    }
    button({55,524,150,40},"MAIN MENU",[this]{show(View::menu);});
}
void App::campaign() {
    const auto& stage=stages[sector];
    shell(stage.art); heading("CAMPAIGN","Sector map",
        std::string(characters[save.character].name)+"  /  "+difficulties[save.difficulty].name+
        "  /  "+std::to_string(save.score)+" lifetime score");
    r.cover(stage.art,{55,171,598,201},1.02f);
    r.gradient({55,171,598,201},hex(0x101920).alpha(.08f),hex(0x09121e).alpha(.79f));
    r.outline({55,171,598,201},hex(stage.accent).alpha(.69f));
    label({70,317},"SECTOR "+digits(sector+1)+"   /   "+stage.threat,hex(stage.accent));
    r.text({69,354},stage.name,39,white,true);
    r.gradient({671,171,474,201},hex(0x182c39),hex(0x101d29));
    r.outline({671,171,474,201},hex(0x465969));
    label({687,197},stage.location,teal);
    copy({687,225},stage.briefing,419);
    label({687,274},std::string("GUARDIAN  /  ")+stage.boss,amber);
    const auto& best=save.records[sector];
    if (best.time>0) label({687,297},"BEST  /  "+std::to_string(best.score)+" SCORE  /  "+timeText(best.time));
    button({687,316,442,40},sector<=save.cleared?"DEPLOY":"CLEAR PREVIOUS SECTOR",[this]{launch(sector);},sector<=save.cleared,true);
    for (int i=0;i<5;++i) {
        const auto& tile=stages[i];
        const Rect box{55.f+221*i,392,206,112};
        r.cover(tile.art,box,1,{.5f,.52f},hex(i>save.cleared?0x75818a:0xc7d7dd));
        r.gradient(box,hex(0x0a141d).alpha(.23f),hex(0x0a141d).alpha(.92f));
        r.outline(box,i==sector?hex(stage.accent):hovered(box)?white:hex(0x4c606d),i==sector?2:1);
        label({box.x+10,box.y+22},"SECTOR "+digits(i+1),pale);
        r.text({box.x+10,box.y+75},tile.name,21,white,true);
        label({box.x+10,box.y+96},i<save.cleared?"CLEARED":i==save.cleared?"AVAILABLE":"LOCKED",i<save.cleared?teal:pale);
        hit(box,[this,i]{sector=i;},true);
    }
    button({55,528,148,40},"MAIN MENU",[this]{show(View::menu);});
    button({972,528,173,40},"ARMORY",[this]{showArmory();},true,true);
    label({790,552},std::to_string(save.credits)+" UPGRADE POINTS",teal);
}
void App::armory() {
    shell("foundry");
    heading("EQUIPMENT","Armory",std::to_string(save.credits)+" upgrade points available");
    for (int i=0;i<3;++i) {
        button({55.f+229*i,171,217,40},weapons[i].name,[this,i]{weapon=i;},true,i==weapon);
    }
    const auto& w=weapons[weapon];
    const Color tint=hex(w.color);
    const bool unlocked=save.cleared>=w.unlock;
    r.gradient({55,226,576,271},hex(0x1a2c37),hex(0x101b26));
    r.outline({55,226,576,271},hex(0x485b68));
    const std::array<const char*,3> art{"carbine","shotgun","arc"};
    r.cover(art[weapon],{56,227,574,150},1,{.5f,.52f});
    r.gradient({56,227,574,150},hex(0x0d1720).alpha(.02f),hex(0x0d1720).alpha(.48f));
    label({69,405},w.type,tint);
    r.text({69,433},w.name,26,white,true);
    r.text({69,459},w.description,15,pale);
    button({55,513,216,41},!unlocked?"CLEAR SECTOR "+digits(w.unlock):weapon==save.equipped?"EQUIPPED":"EQUIP WEAPON",
           [this]{equip();},unlocked && weapon!=save.equipped,true);
    for (int i=0;i<4;++i) {
        const auto& u=upgrades[i];
        const float y=224.f+i*74;
        const int tier=save.tiers[weapon][i], price=u.base+tier*22;
        r.rect({655,y,490,63},hex(0x142632));
        r.outline({655,y,490,63},hex(0x465c6b));
        r.text({667,y+24},u.name,20,white,true);
        r.text({668,y+46},u.description,13,pale);
        for (int j=0;j<3;++j) r.rect({946+j*13.f,y+28,8,8},j<tier?tint:hex(0x405160));
        button({1015,y+12,116,37},tier>=3?"MAX RANK":"BUY  "+std::to_string(price),[this,i]{buy(i);},
            unlocked && tier<3 && save.credits>=price);
    }
    button({972,528,173,39},"CAMPAIGN",[this]{showMap();});
}
void App::options() {
    shell("orbital"); heading("SYSTEM","Settings","Changes are saved as you make them.");
    auto setting=[&](float y,std::string name,std::string value,std::function<void()> action) {
        r.rect({55,y,525,51},hex(0x152531)); r.outline({55,y,525,51},hex(0x415565));
        r.text({70,y+31},name,17,white,true);
        button({385,y+7,179,36},std::move(value),std::move(action));
    };
    auto volume=[&](float y,std::string name,float& gain) {
        r.rect({615,y,530,51},hex(0x152531)); r.outline({615,y,530,51},hex(0x415565));
        r.text({630,y+31},name,17,white,true);
        r.text({988,y+32},std::to_string(int(std::round(gain*100)))+"%",15,white);
        auto* value=&gain;
        button({941,y+7,34,36},"-",[this,value]{*value=clamp(*value-.10f); commitSettings();});
        button({1098,y+7,33,36},"+",[this,value]{*value=clamp(*value+.10f); commitSettings();});
    };
    setting(181,"Game audio",settings.audio?"ON":"OFF",[this]{settings.audio=!settings.audio; commitSettings();});
    setting(242,"Screen shake and camera motion",settings.motion?"ON":"OFF",[this]{settings.motion=!settings.motion; commitSettings();});
    setting(303,"Opening cinematic",settings.cinematic?"ON":"OFF",[this]{settings.cinematic=!settings.cinematic; commitSettings();});
    setting(364,"New campaign difficulty",difficulties[settings.difficulty].name,[this]{settings.difficulty=(settings.difficulty+1)%3; commitSettings();});
    setting(425,"Lighting and bloom",settings.high?"HIGH":"PERFORMANCE",[this]{settings.high=!settings.high; commitSettings();});
    volume(181,"Music",settings.music);
    volume(242,"Effects",settings.effects);
    const bool fs=(SDL_GetWindowFlags(window)&sdl::fullscreen)!=0;
    r.text({630,333},"Window mode",17,white,true);
    button({941,308,190,37},fs?"FULLSCREEN":"WINDOWED",[this]{changeFullscreen();});
    button({55,526,142,41},"BACK",[this]{show(settingsReturn);},true,true);
}
void App::credits() {
    shell("orbital"); heading("VEGA-9","Credits");
    r.gradient({55,172,718,323},hex(0x1b2e39),hex(0x101e29));
    r.outline({55,172,718,323},hex(0x485f6b));
    r.text({74,215},"VOIDLINE  /  Native edition",31,white,true);
    copy({76,251},"Original VOIDLINE campaign, sector art, opening sequence and metal soundtrack.",620,18);
    copy({76,311},"Custom OpenGL renderer with techniques studied in LearnOpenGL, three.js and Godot.",620,17);
    copy({76,370},"Linux platform services by SDL2. Media decoding by libpng, libwebp and libsndfile. Roboto typography under the included license.",620,17);
    label({76,457},"VERSION 2.0.0",teal);
    button({55,526,142,41},"MAIN MENU",[this]{show(View::menu);},true,true);
}
void App::cinematic() {
    struct Cut { const char *art,*tag,*text; float at,bias; };
    constexpr Cut cuts[]{
        {"orbital","VEGA-9  /  LOW ORBIT","The colony went silent.",0,.42f},
        {"descent","MINING SECTOR  /  SIGNAL LOST","Its defenses stayed online.",3.8f,.52f},
        {"cover","FIELD OPERATIONS  /  INSERTION","Reach the orbital uplink.",7.55f,.55f},
        {"terminus","FIELD OPERATIONS","VOIDLINE",11.18f,.43f}
    };
    int i=0;
    while (i<3 && film>=cuts[i+1].at) ++i;
    const auto& c=cuts[i];
    const float t=film-c.at;
    const float opacity=smooth(t/.6f);
    if (i>0 && opacity<1) r.cover(cuts[i-1].art,screen,settings.motion?1.10f:1.f,{.55f,cuts[i-1].bias});
    r.cover(c.art,screen,settings.motion?1.02f+t*.019f:1.f,{.55f,c.bias},white.alpha(opacity));
    r.gradient(screen,hex(0x060b14).alpha(.14f),hex(0x060b14).alpha(.81f));
    r.rect({0,0,width,24},hex(0x050911)); r.rect({0,576,width,24},hex(0x050911));
    label({59,443},c.tag,amber.alpha(opacity));
    r.text({55,516},c.text,i==3?92:54,white.alpha(opacity),true);
    button({1021,520,124,39},"SKIP INTRO",[this]{launch(0);});
}
void App::hud() {
    const auto& p=game.player;
    const auto& g=game.guns[game.gun];
    const auto& w=weapons[game.gun];
    const Color trim=hex(game.character->color);
    const Color accent=hex(game.stage->accent);
    const Color ammo=hex(w.color);
    const Color ink=hex(0x070a10);
    constexpr float slant=12;
    // Angled panels with a bold colored edge, in the spirit of comic-book frames.
    auto panel=[&](Rect b,Color edge) {
        r.slab({b.x-3,b.y+3,b.w,b.h},slant,ink.alpha(.55f),ink.alpha(.55f));
        r.slab(b,slant,hex(0x101a28).alpha(.94f),hex(0x0a111b).alpha(.92f));
        r.slabOutline(b,slant,edge.alpha(.92f),2.2f);
    };
    panel({19,16,271,75},trim);
    label({31,36},game.character->name,trim);
    r.text({31,59},"SUIT  "+std::to_string(p.hp)+" / "+std::to_string(game.character->health),16,white,true);
    for (int i=0;i<10;++i) {
        const Rect cell{31.f+i*24.7f,70,20.7f,7};
        r.slab(cell,3,hex(0x2a3640),hex(0x2a3640));
        const float health=clamp(float(p.hp)*10/game.character->health-i);
        if (health>0) r.slab({cell.x,cell.y,cell.w*health,cell.h},3,p.hp<30?hex(0xff5b45):trim,p.hp<30?hex(0xff5b45):trim);
    }
    // Slow-motion reserve.
    {
        const Rect track{22,98,190,9};
        const bool active=game.slowMotion();
        const Color tint=game.slowEnergy<.22f?hex(0xff8a6a):hex(0x7ff2ff);
        r.slab(track,6,ink.alpha(.8f),ink.alpha(.8f));
        r.slab({track.x,track.y,track.w*game.slowEnergy,track.h},6,tint.energy(active?1.4f:1.f),tint.energy(.7f));
        r.slabOutline(track,6,tint.alpha(.8f),1.2f);
        label({224,107},active?"BULLET TIME":"FOCUS  [Q]",active?tint:muted);
    }
    panel({909,16,272,75},ammo);
    label({921,36},w.name,ammo);
    r.text({922,69},digits(g.ammo),30,white,true);
    r.text({967,67},"/ "+(g.reserve<0?std::string("INF"):std::to_string(g.reserve)),18,pale);
    label({1086,38},"SCORE",muted);
    r.text({1086,67},std::to_string(game.score),24,white,true);
    if (g.reload>0) {
        r.rect({921,77,246,4},hex(0x3e4f5b));
        r.rect({921,77,246*(1-g.reload/g.duration),4},ammo);
    }
    const std::string objective=game.bossDead?"Reach the extraction gate":game.bossActive?"Defeat "+std::string(game.stage->boss):"Find the sector guardian";
    panel({413,16,374,47},accent);
    label({425,35},"SECTOR "+digits(game.index+1),accent);
    r.text({526,37},objective,14,white,true);
    r.rect({425,48,350,3},hex(0x465966));
    r.rect({425,48,350*clamp(p.box.center().x/game.stage->worldWidth),3},accent);
    if (game.bossActive) {
        const auto& boss=game.enemies.back();
        panel({390,72,420,44},hex(0xff8066));
        label({402,91},game.stage->boss,hex(0xff8b71));
        r.rect({402,100,396,6},hex(0x4b333c));
        r.rect({402,100,396*clamp(float(boss.hp)/boss.maxHp),6},hex(0xff8066));
    }
    panel({19,548,282,34},muted);
    label({31,570},p.crouched?"CROUCH":p.sprinting?"SPRINT":p.flip>0?"FLIP":game.checkpoint?"CHECKPOINT ONLINE":"FIELD COMMS  /  ACTIVE",
        p.crouched||p.sprinting||p.flip>0?trim:game.checkpoint?teal:muted);
    r.text({243,570},timeText(game.time),13,white,true);
    panel({802,547,379,36},ammo);
    for (int i=0;i<4;++i) {
        const float x=813.f+i*92;
        if (i==game.gun) r.slab({x-3,553,86,24},5,hex(0x29404b),hex(0x1c2d37));
        r.text({x,569},std::to_string(i+1)+" "+weapons[i].shortName,12,
               !game.guns[i].available?hex(0x647382):i==game.gun?ammo:white,true);
    }
    // Combo counter and style call-outs.
    if (game.combo>=2 && game.comboTimer>0) {
        const float pop=1+.5f*std::exp(-(3.2f-game.comboTimer)*12);
        const Color tint=game.combo>=5?hex(0xff5f7e):game.combo>=3?hex(0xffd35c):hex(0x7ff2ff);
        const std::string count="x"+std::to_string(game.combo);
        r.outlinedText({1105-r.measure(count,54*pop,true)*.5f,170},count,54*pop,tint);
        r.slab({1008,180,170,6},5,ink.alpha(.8f),ink.alpha(.8f));
        r.slab({1008,180,170*clamp(game.comboTimer/3.2f),6},5,tint,tint.energy(.7f));
    }
    if (game.style.life>0) {
        const auto& s=game.style;
        const float age=s.maxLife-s.life;
        const float pop=1+.65f*std::exp(-age*13);
        const float size=30*pop;
        const float alpha=clamp(s.life*3.5f);
        const float x=width*.5f-r.measure(s.text,size,true)*.5f;
        r.outlinedText({x,150-(1-alpha)*10},s.text,size,s.color.alpha(alpha));
    }
    if (game.entry>0) {
        const float a=clamp(game.entry*1.17f);
        label({55,157},game.stage->location,accent.alpha(a));
        r.outlinedText({51,213},game.stage->name,52,white.alpha(a));
        if (game.index==0) {
            r.text({55,243},"SHIFT SPRINT   /   C CROUCH   /   JUMP TWICE TO FLIP   /   Q OR RIGHT CLICK FOR BULLET TIME",12,pale.alpha(a),true);
        }
    }
    if (view==View::play && Rect{0,0,width,height}.contains(mouse) && !input.aimStickActive()) {
        const Color mark=mix(white,ammo,.45f).alpha(.95f);
        const float spin=clock*(game.slowMotion()?2.4f:.6f);
        const float radius=game.slowMotion()?13.f:9.f;
        for (int i=0;i<4;++i) {
            const float a=spin+i*pi*.5f;
            r.line(mouse+ray(a)*(radius-2),mouse+ray(a)*(radius+5),2.4f,ink.alpha(.9f));
            r.line(mouse+ray(a)*(radius-1),mouse+ray(a)*(radius+4),1.2f,mark);
        }
        r.ellipse(mouse,{2.4f,2.4f},ink); r.ellipse(mouse,{1.4f,1.4f},mark);
    }
}
void App::overlay() {
    r.rect(screen,hex(0x070b12).alpha(.72f));
    const Rect panel{300,151,600,291};
    r.gradient(panel,hex(0x172b38),hex(0x0f1d27));
    r.outline(panel,hex(0x5b7480));
    r.rect({panel.x,panel.y,5,panel.h},amber);
    const bool pause=view==View::pause, fail=view==View::failed;
    label({323,181},"SECTOR "+digits(game.index+1)+"  /  "+game.stage->name,amber);
    const std::string title=pause?"Paused":fail?"Suit integrity lost":game.index==4?"Campaign complete":"Sector cleared";
    r.text({321,240},title,48,white,true);
    if (pause) {
        r.text({325,277},"A / D  Move     SPACE  Jump (twice to flip)     SHIFT  Sprint     C  Crouch",15,pale);
        r.text({325,299},"Mouse / F  Fire     Q / Right click  Bullet time     R  Reload     1-4  Equip",15,pale);
        button({325,327,163,41},"RESUME",[this]{closeAction();},true,true);
        button({519,327,163,41},"SETTINGS",[this]{settingsReturn=View::pause; show(View::settings);});
        button({712,327,163,41},"LEAVE MISSION",[this] {
            ask(View::pause,"Leave this mission?","Returning to the sector map ends the current attempt.",[this]{showMap();});
        });
    } else {
        copy({325,274},fail?(game.checkpoint?"Retry from the active checkpoint.":"Retry the sector from its insertion point."):
            game.index==4?"The orbital uplink is online. Vega-9 can call for help again.":"Spend your earned score on upgrades in the armory.",534,17);
        if (!fail) label({325,321},std::to_string(game.score)+" SCORE   /   "+std::to_string(game.kills)+" TARGETS   /   "+timeText(game.time),teal);
        button({325,348,246,42},fail?"RETRY":"ARMORY",[this,fail]{if (fail) retry(); else showArmory();},true,true);
        button({624,348,251,42},"SECTOR MAP",[this]{showMap();});
    }
}
void App::draw() {
    buttons.clear();
    switch (view) {
    case View::menu: menu(); break;
    case View::create: create(); break;
    case View::load: load(); break;
    case View::campaign: campaign(); break;
    case View::armory: armory(); break;
    case View::settings: options(); break;
    case View::credits: credits(); break;
    case View::intro: cinematic(); break;
    case View::play:
    case View::pause:
    case View::failed:
    case View::cleared: {
        const float alpha=view==View::play?clamp(accumulator/step):1.f;
        r.drawWorld(game,settings.high,settings.motion,alpha,previousPlayer,previousCamera);
        hud();
        if (view!=View::play) overlay();
        break;
    }
    case View::confirm:
        shell("orbital");
        r.gradient({300,170,600,260},hex(0x1a2d38),hex(0x12202b));
        r.outline({300,170,600,260},hex(0x587582));
        r.text({323,239},confirmTitle,39,white,true);
        copy({325,276},confirmCopy,535,18);
        button({325,347,224,43},"CANCEL",[this]{show(confirmReturn);});
        button({648,347,227,43},"CONFIRM",[this]{auto action=std::move(confirmAction); action();},true,true);
        break;
    }
    if (notice>0) {
        const float w=std::min(1150.f,r.measure(notification,15)+32);
        r.rect({(width-w)*.5f,5,w,29},hex(0x142630).alpha(.97f));
        r.text({(width-w)*.5f+16,25},notification,15,teal);
    }
}
void App::sceneForTest(std::string_view scene) {
    if (scene=="menu") { show(View::menu); return; }
    if (scene=="create") { newGame(); return; }
    if (scene=="armory") { save.cleared=3; save.credits=280; showArmory(); return; }
    if (scene=="intro") { film=5; show(View::intro); return; }
    // Scripted gameplay scenes exercise the input path with simulated key chords.
    auto prepare=[&](int index,int gun) {
        save.cleared=3; save.equipped=gun;
        launch(index); game.entry=0;
    };
    if (scene.rfind("stand",0)==0) {
        prepare(0,scene.size()>5?scene[5]-'0':0);
        game.pointer({700,420});
        return;
    }
    if (scene=="flip") {
        prepare(0,0);
        game.pointer({900,300});
        script=[this](int frame) {
            input.inject(sdl::key::d,frame<120);
            input.inject(sdl::key::space,(frame>=10 && frame<30) || (frame>=34 && frame<40));
            input.inject(sdl::key::f,frame>=36);
        };
        return;
    }
    if (scene=="slow") {
        prepare(0,0);
        game.pointer({900,420});
        script=[this](int frame) {
            input.inject(sdl::key::d,true);
            input.inject(sdl::key::q,frame>=20);
            input.inject(sdl::key::f,true);
            input.inject(sdl::key::shiftLeft,true);
        };
        return;
    }
    const int index=std::clamp(std::stoi(std::string(scene))-1,0,4);
    launch(index);
    game.entry=0;
    input.inject(sdl::key::d,true);
    input.inject(sdl::key::f,true);
}
}
