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
    const Rect panel{55,177,1090,357};
    r.gradient(panel,hex(0x172b38),hex(0x0d1b27));
    r.outline(panel,hex(0x496573));

    if (lobby!=Lobby::menu && (network.state()==State::connected || rosterPreview) && mission==0) {
        const bool host=network.hosting();
        label({80,210},std::string("SQUAD ASSEMBLY  /  ")+(host?"HOST":"GUEST"),teal);
        r.text({81,242},"Choose your operative",29,white,true);
        label({664,219},std::string("YOU: ")+(characterLocked?"LOCKED IN":"SELECTING"),characterLocked?teal:amber);
        label({876,219},std::string("PARTNER: ")+(remoteLocked?"READY":"SELECTING"),remoteLocked?teal:amber);
        for (int i=0;i<static_cast<int>(characters.size());++i) {
            const auto& c=characters[i];
            const Rect card{76.f+(i%3)*353.f,260.f+(i/3)*105.f,344,98};
            const bool chosen=i==coopCharacter;
            r.rect(card,hex(0x091620));
            r.cover(c.art,{card.x+2,card.y+2,80,card.h-4},1,{.5f,.28f});
            r.gradient({card.x+66,card.y,30,card.h},hex(0x0b151e).alpha(0),hex(0x0b151e));
            const Color tint=hex(c.color);
            label({card.x+96,card.y+24},c.role,tint);
            r.text({card.x+96,card.y+53},c.name,21,white,true);
            r.text({card.x+96,card.y+77},c.trait,11,pale);
            r.outline(card,chosen?tint:hex(0x445968),chosen?2.6f:1.f);
            hit(card,[this,i]{if (!characterLocked) {coopCharacter=i;audio.event("switch",.6f);}},!characterLocked);
        }
        label({82,505},characterLocked?"WAITING FOR PARTNER TO LOCK IN":"SELECT ONE OF SIX OPERATIVES",characterLocked?teal:amber);
        button({846,484,232,44},characterLocked?"READY  /  LOCKED":"LOCK IN CHARACTER",
            [this]{lockOperative();},!characterLocked,true);
    } else if (lobby==Lobby::menu) {
        r.text({81,245},"Host a mission",32,white,true);
        copy({81,279},"Set the difficulty and share your session key. Both players deploy together in Chapter 1.",439,18);
        button({81,359,401,48},"HOST GAME",[this]{lobby=Lobby::host; focus=-1; SDL_StartTextInput();},true,true);
        r.line({591,208},{591,470},1,hex(0x496573));
        r.text({630,245},"Join a mission",32,white,true);
        copy({630,279},"Paste a session key, or search for nearby LAN hosts and request to join.",439,18);
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
        copy({83,322},"Share the key or let a player discover this host on the LAN. All join requests require approval.",945,17);
        r.rect({83,353,730,58},hex(0x091722));
        r.outline({83,353,730,58},hex(0x547582));
        r.text({99,389},network.key(),22,white,true);
        button({837,353,241,58},"COPY KEY",[this] {
            if (SDL_SetClipboardText(network.key().c_str())==0) inform("Session key copied.");
            else inform("Could not copy the key. You can type it on the other computer.");
        },true,true);
        if (network.pendingApproval()) {
            r.rect({83,423,994,94},hex(0x20303c));
            r.outline({83,423,994,94},amber);
            label({97,448},"PLAYER JOIN REQUEST  /  "+network.joiningAddress(),amber);
            r.text({97,474},"Allow this player to join your mission?",18,white,true);
            button({668,460,176,43},"DECLINE",[this]{network.approve(false);});
            button({856,460,176,43},"ACCEPT",[this]{network.approve(true);},true,true);
        } else {
            label({83,458},network.state()==State::connected?"2 / 2 PLAYERS CONNECTED":
                  (network.discoverable()?"1 / 2 PLAYERS  /  LAN VISIBLE":"1 / 2 PLAYERS  /  KEY ONLY"),amber);
        }
    } else {
        const bool editable=network.state()==State::idle;
        label({83,219},"SESSION KEY  /  DIRECT CONNECT",teal);
        r.rect({83,233,740,52},hex(0x091722));
        r.outline({83,233,740,52},hex(0x547582));
        r.text({97,265},joinKey.empty()?"Paste or type the host's key":joinKey,20,joinKey.empty()?muted:white,true);
        button({836,233,241,52},"PASTE KEY",[this] {
            char* clipboard=SDL_GetClipboardText();
            if (clipboard) { joinKey.clear(); appendField(joinKey,clipboard,false); SDL_free(clipboard); }
        },editable);
        button({83,299,286,42},editable?"CONNECT WITH KEY":"WAIT FOR APPROVAL",[this] {
            if (network.join(joinKey)) { focus=-1; SDL_StopTextInput(); }
        },editable && !joinKey.empty(),true);
        button({555,299,522,42},"FIND LOCAL HOSTS  /  REFRESH",[this] {
            lanPage=0;
            if (!network.searchLan()) inform("LAN discovery could not start. Check firewall permissions.");
        },editable);
        label({83,365},"AVAILABLE HOSTS ON YOUR LOCAL NETWORK",teal);
        const auto& hosts=network.nearby();
        if (hosts.empty()) copy({83,395},network.state()==State::approval?
            "Request sent. Waiting for host approval...":
            "No hosts found yet. Select FIND LOCAL HOSTS and allow UDP 38275 through the firewall.",920,17);
        else for (size_t index=static_cast<size_t>(lanPage*3);index<hosts.size() && index<static_cast<size_t>((lanPage+1)*3);++index) {
            const auto entry=hosts[index];
            const float y=380+(index-static_cast<size_t>(lanPage*3))*42;
            r.rect({83,y,996,38},hex(0x102632));
            r.outline({83,y,996,38},hex(0x3b5362));
            r.text({96,y+24},entry.name+"   /   "+entry.address+":"+std::to_string(entry.port),16,pale,true);
            button({879,y+2,196,34},"REQUEST JOIN",[this,entry] {
                if (network.joinLan(entry)) { focus=-1; SDL_StopTextInput(); }
            },editable,true);
        }
        if (hosts.size()>3) {
            button({83,510,155,30},"PREVIOUS",[this]{lanPage=std::max(0,lanPage-1);},lanPage>0);
            button({250,510,155,30},"NEXT HOSTS",[this]{lanPage=std::min(int((network.nearby().size()-1)/3),lanPage+1);},(lanPage+1)*3<int(hosts.size()));
        }

    }
    if (!network.error().empty()) copy({440,550},network.error(),700,15,amber);
    button({55,545,160,41},lobby==Lobby::menu?"MAIN MENU":"BACK",[this]{closeAction();});
}

void App::coopOverlay() {
    const bool playing=view==View::play;
    const bool cleared=view==View::cleared;
    r.rect(screen,hex(0x070b12).alpha(.72f));
    r.gradient({300,151,600,291},hex(0x172b38),hex(0x0f1d27));
    r.outline({300,151,600,291},hex(0x5b7480));
    label({323,181},"CO-OP  /  SECTOR "+digits(game.index+1),amber);
    r.text({321,240},playing?"Co-op menu":cleared?(game.index==sectorCount-1?"Campaign complete":"Sector cleared"):"Squad down",46,white,true);
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
