#include "support.hpp"
#include "net/world.hpp"
#include "net/pause.hpp"
#include <chrono>
#include <thread>

using namespace vl;
using namespace test;

namespace {
bool until(net::Session& host,net::Session& guest,const auto& condition) {
    // Allow for Windows loopback scheduling and antivirus delays while still
    // requiring prompt connection rejection and handshake completion.
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    do {
        host.poll(); guest.poll();
        if (condition()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now()<deadline);
    return false;
}

void handshake() {
    using State=net::Session::State;
    net::Session host,wrong,guest,third;
    CHECK(host.host("127.0.0.1",0));
    if (host.state()!=State::waiting) {
        std::cerr<<"Host setup: "<<host.error()<<'\n';
        return;
    }
    const auto key=host.key();
    CHECK(key.starts_with("VL12-") && key.size()==41 && host.receive().empty());
    CHECK(!wrong.join("not-a-session-key"));
    for (const auto old:{"VL1-","VL2-","VL3-","VL4-","VL5-","VL6-","VL7-","VL8-","VL9-"}) {
        CHECK(!wrong.join(std::string(old)+key.substr(5)));
        CHECK(wrong.error().find("2.10.6")!=std::string::npos);
    }
    auto wrongKey=key;
    wrongKey.back()=wrongKey.back()=='0'?'1':'0';
    CHECK(wrong.join(wrongKey));
    CHECK(until(host,wrong,[&] { return host.state()==State::waiting && wrong.state()==State::idle; }));
    CHECK(host.key()==key && host.receive().empty());

    CHECK(guest.searchLan());
    CHECK(until(host,guest,[&] { return !guest.nearby().empty(); }));
    const auto found=guest.nearby();
    CHECK(!found.empty());
    if (!found.empty()) {
        CHECK(!found[0].address.empty() && found[0].port>0);
        CHECK(found[0].name.size()>0 && found[0].name.size()<=25);
        CHECK(found[0].name.find(key)==std::string::npos); // No private key in a broadcast.
        CHECK(guest.joinLan(found[0]));
        CHECK(until(host,guest,[&] { return host.pendingApproval(); }));
        CHECK(!host.joiningAddress().empty());
        CHECK(host.state()==State::approval && guest.state()==State::approval);
        CHECK(host.receive().empty() && guest.receive().empty());
        host.approve(false);
        CHECK(until(host,guest,[&] { return guest.state()==State::idle && host.state()==State::waiting; }));
        CHECK(guest.error().find("declined")!=std::string::npos);
    }

    CHECK(guest.join(key));
    CHECK(until(host,guest,[&] { return host.pendingApproval(); }));
    CHECK(host.state()==State::approval && guest.state()==State::approval);
    host.approve(true);
    CHECK(until(host,guest,[&] { return host.state()==State::connected && guest.state()==State::connected; }));
    CHECK(host.hosting() && !guest.hosting());
    CHECK(host.receive().empty() && guest.receive().empty());
    // The host's Cooperative/Campaign mode advertisement must pass through
    // the shared packet validator. This was previously rejected as > pauseState.
    CHECK(host.send(net::Packet::lobbyMode,std::array<uint8_t,1>{1}));
    std::vector<net::Bytes> messages;
    CHECK(until(host,guest,[&] { messages=guest.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages[0].size()==2 &&
          messages[0][0]==static_cast<uint8_t>(net::Packet::lobbyMode) && messages[0][1]==1);
    CHECK(guest.send(net::Packet::operative,std::array<uint8_t,1>{5}));
    CHECK(until(host,guest,[&] { messages=host.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages.front().front()==static_cast<uint8_t>(net::Packet::operative) && messages.front().size()==2 && messages.front()[1]==5);
    // Each side independently announces its own lock-in; the selection must
    // survive the wire without being replaced by a default character ID.
    CHECK(host.send(net::Packet::operative,std::array<uint8_t,1>{3}));
    CHECK(until(host,guest,[&] { messages=guest.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages.front().size()==2 &&
          messages.front()[0]==static_cast<uint8_t>(net::Packet::operative) && messages.front()[1]==3);
    // Both operating systems use these same frame types for synchronized skip.
    CHECK(guest.send(net::Packet::skipRequest));
    CHECK(until(host,guest,[&] { messages=host.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages.front().size()==1 &&
          messages.front()[0]==static_cast<uint8_t>(net::Packet::skipRequest));
    CHECK(host.send(net::Packet::skipOpening,net::encodeTiming({1,123450,0})));
    CHECK(until(host,guest,[&] { messages=guest.receive(); return !messages.empty(); }));
    net::Timing skip{};
    CHECK(messages.size()==1 && messages.front().size()>1 &&
          messages.front()[0]==static_cast<uint8_t>(net::Packet::skipOpening) &&
          net::decodeTiming(std::span<const uint8_t>(messages.front()).subspan(1),skip) &&
          skip.mission==1 && skip.first==123450 && skip.second==0);
    // Cross-platform pause requests and state broadcasts use fixed-width little-endian fields.
    net::PauseState frozen{true,net::PauseOwner::guest};
    const auto pauseReq=net::encodePauseRequest(7,true);
    uint32_t generation=0;
    bool requested=false;
    CHECK(net::decodePauseRequest(pauseReq,generation,requested) && generation==7 && requested);
    CHECK(!net::decodePauseRequest({pauseReq.data(),4},generation,requested));
    CHECK(guest.send(net::Packet::pauseRequest,pauseReq));
    CHECK(until(host,guest,[&] { messages=host.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages[0][0]==static_cast<uint8_t>(net::Packet::pauseRequest));
    CHECK(net::decodePauseRequest(std::span(messages[0]).subspan(1),generation,requested) && generation==7 && requested);
    CHECK(host.send(net::Packet::pauseState,net::encodePauseState(7,frozen)));
    CHECK(until(host,guest,[&] { messages=guest.receive(); return !messages.empty(); }));
    net::PauseState confirmed{};
    CHECK(messages.size()==1 && messages[0][0]==static_cast<uint8_t>(net::Packet::pauseState));
    CHECK(net::decodePauseState(std::span(messages[0]).subspan(1),generation,confirmed));
    CHECK(generation==7 && confirmed.active && confirmed.owner==net::PauseOwner::guest);
    CHECK(!net::changePause(confirmed,net::PauseOwner::host,false));
    CHECK(confirmed.active);
    CHECK(net::changePause(confirmed,net::PauseOwner::guest,false) && !confirmed.active);
    CHECK(!net::changePause(confirmed,net::PauseOwner::guest,false));
    CHECK(net::changePause(confirmed,net::PauseOwner::host,true) && confirmed.active);
    CHECK(!net::changePause(confirmed,net::PauseOwner::guest,true));
    CHECK(net::changePause(confirmed,net::PauseOwner::host,false) && !confirmed.active);
    // A full session must immediately reject extra connections on Windows
    // as well as Linux, without disrupting the accepted guest.
    CHECK(third.join(key));
    CHECK(until(host,third,[&] { return third.state()==State::idle; }));
    CHECK(host.state()==State::connected && guest.state()==State::connected);
    CHECK(!host.discoverable());
    CHECK(guest.send(net::Packet::operative,std::array<uint8_t,1>{4}));
    CHECK(until(host,guest,[&] { messages=host.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages[0].size()==2 && messages[0][1]==4);
    // Repeated attempts must be rejected, not leak pending sockets.
    for (int attempt=0;attempt<3;++attempt) {
        CHECK(third.join(key));
        CHECK(until(host,third,[&] { return third.state()==State::idle; }));
    }
    CHECK(host.state()==State::connected && guest.state()==State::connected);
    guest.close();
    CHECK(until(host,guest,[&] { return host.state()==State::idle; }));
    CHECK(!host.error().empty());
    CHECK(host.host("127.0.0.1",0));
    CHECK(host.key()!=key);
}

void sharedSimulation() {
    Silent audio;
    Game host(audio),guest(audio);
    auto loadout=fullSave(); loadout.character=3;
    host.startCoop(0,loadout,5);
    guest.startCoop(0,loadout,5);
    CHECK(host.character==&characters[3] && host.teammate().character==&characters[5]);
    const auto hostStart=host.player.box.x;
    const auto guestStart=host.teammate().player.box.x;
    for (auto& enemy:host.enemies) enemy.damage=0;
    Controls move;
    move.held=keyBit(Key::right);
    host.applyControls(move,true);
    run(host,30);
    CHECK(host.player.box.x==hostStart && host.teammate().player.box.x>guestStart+50);
    host.applyControls({},true);
    const auto guestX=host.teammate().player.box.x;
    move.held=keyBit(Key::left);
    host.applyControls(move);
    run(host,20);
    CHECK(host.player.box.x<hostStart && host.teammate().player.box.x<guestX+35);

    const float floor=host.teammate().player.box.bottom();
    Controls jump;
    jump.held=jump.pressed=keyBit(Key::jump);
    host.applyControls(jump,true);
    run(host,25);
    CHECK(host.teammate().player.box.bottom()<floor-30 && host.player.support>=0);

    Controls shoot;
    shoot.pressed=keyBit(Key::fire); shoot.held=keyBit(Key::fire);
    shoot.weapon=6;
    shoot.aimMode=AimMode::pointer; shoot.aim={400,475};
    host.applyControls(shoot,true);
    run(host,25);
    CHECK(host.gun==0 && host.teammate().gun==6);
    CHECK(host.guns[0].ammo==host.guns[0].capacity && host.teammate().guns[6].ammo<host.teammate().guns[6].capacity);

    const auto hostHP=host.player.hp;
    auto partner=host.teammate();
    partner.player.box={1260,449,32,56};
    partner.player.safe={1260,505};
    partner.player.flash=0; partner.player.velocity={};
    host.setTeammate(partner);
    host.applyControls({},true);
    host.hostile.push_back({{1235,475},{650,0},.2f,16,hex(0xffffff)});
    run(host,12);
    CHECK(host.player.hp==hostHP && host.teammate().player.hp<partner.player.hp);
    CHECK(host.teammate().camera>host.camera+100);

    const auto world=net::encodeWorld(host,{{"jump",1,0,1}},{2,47});
    net::WorldFrame frame;
    std::vector<net::Sound> sounds;
    CHECK(net::decodeWorld(world,guest,sounds,&frame));
    CHECK(frame.mission==2 && frame.tick==47);
    CHECK(guest.player.box.x==host.teammate().player.box.x && guest.teammate().player.box.x==host.player.box.x);
    CHECK(guest.camera==host.teammate().camera && guest.cameraY==host.teammate().cameraY &&
          guest.player.hp==host.teammate().player.hp);
    CHECK(guest.gun==host.teammate().gun && guest.enemies.size()==host.enemies.size() && sounds.size()==1);
    CHECK(!net::decodeWorld(std::span(world).first(world.size()-1),guest,sounds));
    Controls decoded;
    uint32_t mission=0;
    CHECK(net::decodeControls(net::encodeControls(shoot,2),decoded,mission));
    CHECK(mission==2 && decoded.weapon==6 && decoded.pressed==keyBit(Key::fire));
    CHECK(!net::decodeControls(net::encodeControls({},0),decoded,mission));

    const Vec beacon=host.stage->beacons.front();
    partner.player.box={beacon.x-16,beacon.y-56,32,56};
    partner.player.safe={beacon.x-16,beacon.y};
    host.setTeammate(partner);
    run(host,1);
    CHECK(host.activeBeacon==0);
    host.retry();
    CHECK(host.coop && host.player.hp==host.maxHealth && host.teammate().player.hp==host.teammate().maxHealth);

    for (auto& enemy:host.enemies) enemy.alive=false;
    host.bossDead=true;
    const auto exit=host.stage->exit;
    host.player.box={exit.x-16,exit.y-56,32,56};
    host.player.velocity={};
    host.player.support=-1;
    run(host,1);
    CHECK(host.mode==Mode::active);
    partner=host.teammate();
    partner.player.box={exit.x-16,exit.y-56,32,56};
    partner.player.velocity={}; partner.player.support=-1;
    host.setTeammate(partner);
    run(host,1);
    CHECK(host.mode==Mode::complete);
}
}
void synchronizedClock() {
    net::HostClock clock;
    CHECK(!clock.ready());
    CHECK(clock.sample(1000,1040,9020));
    CHECK(clock.ready() && clock.estimate(1250)==9250);
    CHECK(clock.sample(2000,2190,10020));
    CHECK(clock.estimate(1250)==9250);
    CHECK(clock.sample(3000,3008,11006));
    CHECK(clock.estimate(1250)==9252 && clock.lead()>=350);
    CHECK(!clock.sample(4000,3999,12000));
    net::Timing timing;
    const auto plan=net::encodeTiming({1,9300,9300+net::openingDuration});
    CHECK(net::decodeTiming(plan,timing));
    CHECK(timing.mission==1 && timing.first==9300 && timing.second==9300+net::openingDuration);
    CHECK(!net::decodeTiming(std::span(plan).first(plan.size()-1),timing));
}
void cooperativePuzzles() {
    Silent audio;
    const auto save=fullSave();
    for (int i=0;i<cooperativeSectorCount;++i) {
        const auto& s=cooperativeStages[i];
        CHECK(!s.relays.empty() && s.relays.size()==size_t(4+i));
        CHECK(s.relays.front().barrier.x>700.f);
        CHECK(s.worldWidth>s.exit.x && s.exit.y==s.ground.front().y);
        // The shared landing is clear, with a gradual accessible climb to
        // the higher route and room for two full-size characters below it.
        CHECK(s.ground.front().x==0.f && s.ground.front().w>=400.f);
        CHECK(std::any_of(s.ledges.begin(),s.ledges.end(),[](const Rect& p){
            return p.x<220.f && p.y>430.f && p.y<480.f;
        }));
        CHECK(std::any_of(s.ledges.begin(),s.ledges.end(),[](const Rect& p){
            return p.x>=280.f && p.x<380.f && p.y>370.f && p.y<410.f;
        }));
        CHECK(std::any_of(s.ledges.begin(),s.ledges.end(),[](const Rect& p){
            return p.x>370.f && p.y==326.f;
        }));
        CHECK(s.enemies.back().kind==EnemyKind::boss);
        for (size_t j=0;j<s.relays.size();++j) {
            CHECK(s.relays[j].operatorPlayer==int(j%2==0?0:1));
            CHECK(s.relays[j].pad.x<s.relays[j].barrier.x);
            CHECK(s.relays[j].control==(j%3==1?RelayControl::lever:RelayControl::pressurePlate));
        }
        if (i>0) CHECK(s.worldWidth>cooperativeStages[i-1].worldWidth);
    }
    Game host(audio),guest(audio);
    host.startCoop(0,save,3,true);
    guest.startCoop(0,save,3,true);
    CHECK(host.coop && host.puzzleMode && host.stage==&cooperativeStages[0]);
    CHECK(host.player.box.bottom()==512.f);
    CHECK(host.teammate().player.box.bottom()==512.f);
    CHECK(std::abs(host.player.box.center().x-host.teammate().player.box.center().x)<=55.f);
    CHECK(host.player.support>=0 && host.teammate().player.support>=0);
    CHECK(host.player.safe.y==host.teammate().player.safe.y);
    for (const auto& enemy:host.enemies)
        CHECK(enemy.box.x>host.teammate().player.box.right()+250.f);
    CHECK(host.enemies.size()==host.stage->enemies.size());
    const auto& first=host.stage->relays[0];
    // An unpowered gate is not bypassable by running or double jumping.
    Game blocked(audio);
    blocked.startCoop(0,save,3,true);
    for (auto& enemy:blocked.enemies) enemy.alive=false;
    auto runner=blocked.teammate();
    runner.player.box={first.barrier.x-42.f,first.barrier.bottom()-56.f,32,56};
    runner.player.velocity={}; runner.player.support=0;
    blocked.setTeammate(runner);
    Controls tryGate;
    tryGate.held=keyBit(Key::right)|keyBit(Key::jump);
    tryGate.pressed=keyBit(Key::jump);
    blocked.applyControls(tryGate,true);
    run(blocked,90);
    CHECK(blocked.teammate().player.box.right()<=first.barrier.x+.1f);
    CHECK(!blocked.relayOpen(0));
    CHECK(first.operatorPlayer==0 && !host.relayOpen(0));
    host.player.box={first.pad.x-16.f,first.pad.y-56.f,32,56};
    host.updateRelays(.8f);
    CHECK(host.relayOpen(0) && !host.relayStates[0].latched);
    host.player.box.x=first.pad.x-150.f;
    host.updateRelays(.5f);
    CHECK(!host.relayOpen(0));
    host.player.box.x=first.pad.x-16.f;
    host.updateRelays(.8f);
    auto partner=host.teammate();
    partner.player.box={first.barrier.right()+60.f,first.barrier.bottom()-56.f,32,56};
    host.setTeammate(partner);
    host.updateRelays(.02f);
    CHECK(host.relayStates[0].latched);
    host.player.box.x=first.pad.x-150.f;
    host.updateRelays(.9f);
    CHECK(host.relayOpen(0));
    const auto& second=host.stage->relays[1];
    CHECK(second.operatorPlayer==1 && second.control==RelayControl::lever);
    partner=host.teammate();
    partner.player.box={second.pad.x-16.f,second.pad.y-56.f,32,56};
    host.setTeammate(partner);
    host.updateRelays(.7f);
    CHECK(!host.relayOpen(1)); // Standing beside the lever is not enough.
    Controls pull;
    pull.held=keyBit(Key::interact);
    host.applyControls(pull,true);
    host.updateRelays(.6f);
    CHECK(host.relayOpen(1) && host.relayStates[1].lease>0);
    host.applyControls({},true);
    host.updateRelays(.9f);
    CHECK(host.relayOpen(1)); // Lever keeps the door open temporarily.
    host.updateRelays(7.f);
    CHECK(!host.relayOpen(1) && host.relayStates[1].lease==0.f);
    host.applyControls(pull,true);
    host.updateRelays(.6f);
    CHECK(host.relayOpen(1));
    host.applyControls({},true);
    host.player.box={second.barrier.right()+60.f,second.barrier.bottom()-56.f,32,56};
    host.updateRelays(.02f);
    CHECK(host.relayStates[1].latched && host.relayStates[1].openness>.8f);
    const auto packet=net::encodeMission(3,1,7,CoopMode::cooperative);
    int index=-1,level=-1; uint32_t mission=0;
    CoopMode mode=CoopMode::campaign;
    CHECK(net::decodeMission(packet,index,level,mission,&mode));
    CHECK(index==3 && level==1 && mission==7 && mode==CoopMode::cooperative);
    CHECK(!net::decodeMission(std::span(packet).first(packet.size()-1),index,level,mission,&mode));
    const auto frame=net::encodeWorld(host,{}, {7,19});
    std::vector<net::Sound> sounds;
    CHECK(net::decodeWorld(frame,guest,sounds));
    CHECK(guest.puzzleMode && guest.relayStates.size()==host.relayStates.size());
    CHECK(guest.relayStates[0].latched && guest.relayStates[1].latched);
    CHECK(guest.relayOpen(0) && guest.relayOpen(1));
    CHECK(guest.relayStates[1].openness==host.relayStates[1].openness);
    CHECK(guest.relayStates[1].lease==host.relayStates[1].lease);
    // Checkpoint retries must ALSO regroup both players without dropping the
    // host on an isolated gantry. Re-entry steps exist beside checkpoints.
    const auto checkpoint=host.stage->beacons.front();
    CHECK(std::any_of(host.stage->ledges.begin(),host.stage->ledges.end(),[&](const Rect& p){
        return p.x>checkpoint.x && p.x<checkpoint.x+150.f && p.y>430.f && p.y<470.f;
    }));
    host.player.box={checkpoint.x-16.f,checkpoint.y-56.f,32.f,56.f};
    host.player.support=0;
    host.player.velocity={};
    for (auto& enemy:host.enemies) enemy.alive=false;
    host.tick(); // production checkpoint trigger, not a test-only shortcut
    CHECK(host.activeBeacon==0);
    host.retry();
    CHECK(host.player.box.bottom()==checkpoint.y);
    CHECK(host.teammate().player.box.bottom()==checkpoint.y);
    CHECK(std::abs(host.player.box.center().x-host.teammate().player.box.center().x)<100.f);
}

void runCoopTests() {
    synchronizedClock();
    handshake();
    sharedSimulation();
    cooperativePuzzles();
}
