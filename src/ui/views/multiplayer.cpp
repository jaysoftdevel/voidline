#include "ui/app.hpp"
#include "ui/common.hpp"
#include <cctype>

using namespace vl::ui;

namespace vl {
namespace {
void appendField(std::string& field,std::string_view text,bool address) {
    const size_t limit=address?15:40;
    for (const unsigned char c:text) {
        if (field.size()>=limit) break;
        if (address?(std::isdigit(c) || c=='.'):(std::isalnum(c) || c=='-'))
            field+=static_cast<char>(std::toupper(c));
    }
}
}

bool App::lobbyInput(const SDL_Event& e) {
    if (lobby==Lobby::menu || network.state()!=net::Session::State::idle) return false;
    const bool address=lobby==Lobby::host;
    auto& field=address?advertisedAddress:joinKey;
    if (e.type==sdl::textInput) {
        const auto end=std::find(std::begin(e.text.text),std::end(e.text.text),'\0');
        appendField(field,std::string_view(e.text.text,end),address);
        return true;
    }
    if (e.type!=sdl::keyDown) return false;
    const int key=e.key.keysym.scancode;
    if (key==sdl::key::backspace) {
        if (!field.empty()) field.pop_back();
        return true;
    }
    constexpr unsigned ctrl=0x00c0;
    if ((e.key.keysym.mod&ctrl) && key==sdl::key::a) { field.clear(); return true; }
    if ((e.key.keysym.mod&ctrl) && key==sdl::key::v) {
        char* clipboard=SDL_GetClipboardText();
        if (clipboard) { field.clear(); appendField(field,clipboard,address); SDL_free(clipboard); }
        return true;
    }
    return false;
}

void App::multiplayer() {
    using State=net::Session::State;
    shell("orbital");
    heading("MULTIPLAYER","Two-player co-op","Play the same mission with separate controls and cameras.");
    const Rect panel{55,177,1090,339};
    r.gradient(panel,hex(0x172b38),hex(0x0d1b27));
    r.outline(panel,hex(0x496573));

    if (lobby==Lobby::menu) {
        r.text({81,245},"Host a mission",32,white,true);
        copy({81,279},"Set the difficulty and share your session key. Both players deploy together in Chapter 1.",439,18);
        button({81,359,401,48},"HOST GAME",[this]{lobby=Lobby::host; focus=-1; SDL_StartTextInput();},true,true);
        r.line({591,208},{591,470},1,hex(0x496573));
        r.text({630,245},"Join a mission",32,white,true);
        copy({630,279},"Paste the host's session key to join their mission.",439,18);
        button({630,359,401,48},"JOIN GAME",[this]{lobby=Lobby::join; focus=-1; SDL_StartTextInput();});
    } else if (lobby==Lobby::host && network.state()==State::idle) {
        label({83,225},"CAMPAIGN START",teal);
        r.text({83,268},"Chapter 1. The Descent",28,white,true);
        label({83,322},"DIFFICULTY",teal);
        button({83,336,52,42},"<",[this]{--coopDifficulty;},coopDifficulty>0);
        button({442,336,52,42},">",[this]{++coopDifficulty;},coopDifficulty+1<static_cast<int>(difficulties.size()));
        r.text({155,363},difficulties[coopDifficulty].name,20,white,true);
        label({555,225},"HOST IPv4 ADDRESS",teal);
        r.rect({555,240,550,42},hex(0x091722));
        r.outline({555,240,550,42},hex(0x547582));
        r.text({568,268},advertisedAddress,20,white,true);
        copy({555,322},"Use an address your partner can reach over the local network or a VPN.",525,16);
        button({555,369,284,45},"GENERATE SESSION KEY",[this] {
            if (network.host(advertisedAddress)) { focus=-1; SDL_StopTextInput(); }
        },true,true);
    } else if (lobby==Lobby::host) {
        label({83,226},"CHAPTER 1  /  THE DESCENT",teal);
        r.text({81,284},"Waiting for player 2",37,white,true);
        copy({83,322},"Send the session key to your partner. The opening begins once both players are ready.",880,17);
        r.rect({83,353,730,58},hex(0x091722));
        r.outline({83,353,730,58},hex(0x547582));
        r.text({99,389},network.key(),22,white,true);
        button({837,353,241,58},"COPY KEY",[this] {
            if (SDL_SetClipboardText(network.key().c_str())==0) inform("Session key copied.");
            else inform("Could not copy the key. You can type it on the other computer.");
        },true,true);
        label({83,458},"1 / 2 PLAYERS CONNECTED",amber);
    } else {
        const bool editable=network.state()==State::idle;
        label({83,226},"SESSION KEY",teal);
        r.rect({83,245,743,61},hex(0x091722));
        r.outline({83,245,743,61},hex(0x547582));
        r.text({99,284},joinKey.empty()?"Paste or type the host's key":joinKey,22,joinKey.empty()?muted:white,true);
        button({849,245,228,61},"PASTE KEY",[this] {
            char* clipboard=SDL_GetClipboardText();
            if (clipboard) { joinKey.clear(); appendField(joinKey,clipboard,false); SDL_free(clipboard); }
        },editable);
        button({83,343,284,45},editable?"CONNECT":"CONNECTING",[this] {
            if (network.join(joinKey)) { focus=-1; SDL_StopTextInput(); }
        },editable && !joinKey.empty(),true);
    }
    if (!network.error().empty()) copy({83,482},network.error(),1010,16,amber);
    button({55,534,160,41},lobby==Lobby::menu?"MAIN MENU":"BACK",[this]{closeAction();});
}

void App::coopOverlay() {
    const bool playing=view==View::play;
    const bool cleared=view==View::cleared;
    r.rect(screen,hex(0x070b12).alpha(.72f));
    r.gradient({300,151,600,291},hex(0x172b38),hex(0x0f1d27));
    r.outline({300,151,600,291},hex(0x5b7480));
    label({323,181},"CO-OP  /  SECTOR "+digits(game.index+1),amber);
    r.text({321,240},playing?"Co-op menu":cleared?"Sector cleared":"Squad down",46,white,true);
    if (playing) {
        copy({325,283},"Your partner can keep playing while this menu is open.",532,17);
        button({325,344,249,45},"RESUME",[this]{closeAction();},true,true);
    } else {
        const bool final=cleared && game.index==sectorCount-1;
        const std::string message=final?"The emergency uplink is online.":network.hosting()?
            (cleared?"Both operatives reached extraction.":"Retry together from the last checkpoint or insertion point."):
            "The host can start the next attempt.";
        copy({325,283},message,533,17);
        if (cleared) label({325,322},std::to_string(game.score)+" SCORE   /   "+timeText(game.time),teal);
        if (!final && network.hosting()) button({325,344,249,45},cleared?"NEXT SECTOR":"RETRY",
            [this,cleared]{startCoop(game.index+int(cleared),!cleared);},true,true);
    }
    button({624,344,251,45},"LEAVE SESSION",[this]{leaveCoop();});
}
}
