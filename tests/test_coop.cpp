#include "support.hpp"
#include "net/world.hpp"
#include <chrono>
#include <thread>

using namespace vl;
using namespace test;

namespace {
bool until(net::Session& host,net::Session& guest,const auto& condition) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
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
    if (!host.host("127.0.0.1",0)) {
        std::cerr<<"Co-op host test: "<<host.error()<<'\n';
        CHECK(false);
        return;
    }
    const auto key=host.key();
    CHECK(key.starts_with("VL4-") && key.size()==40 && host.state()==State::waiting && host.receive().empty());
    CHECK(!wrong.join("not-a-session-key"));
    CHECK(!wrong.join("VL1-"+key.substr(4)));
    CHECK(wrong.error().find("2.9")!=std::string::npos);
    CHECK(!wrong.join("VL2-"+key.substr(4)));
    CHECK(!wrong.join("VL3-"+key.substr(4)));
    auto wrongKey=key; wrongKey.back()=wrongKey.back()=='0'?'1':'0';
    CHECK(wrong.join(wrongKey));
    CHECK(until(host,wrong,[&] { return host.state()==State::waiting && wrong.state()==State::idle; }));
    CHECK(host.key()==key && host.receive().empty());
    CHECK(guest.join(key));
    CHECK(until(host,guest,[&] { return host.state()==State::connected && guest.state()==State::connected; }));
    CHECK(host.hosting() && !guest.hosting());
    CHECK(host.receive().empty() && guest.receive().empty());
    CHECK(guest.send(net::Packet::ready));
    std::vector<net::Bytes> messages;
    CHECK(until(host,guest,[&] { messages=host.receive(); return !messages.empty(); }));
    CHECK(messages.size()==1 && messages.front().front()==static_cast<uint8_t>(net::Packet::ready));
    third.join(key);
    CHECK(until(host,third,[&] { return third.state()==State::idle; }));
    CHECK(host.state()==State::connected);
    guest.close();
    CHECK(until(host,guest,[&] { return host.state()==State::idle; }));
    CHECK(!host.error().empty());
    CHECK(host.host("127.0.0.1",0));
    CHECK(host.key()!=key);
}

void sharedSimulation() {
    Silent audio;
    Game host(audio),guest(audio);
    host.startCoop(0,fullSave());
    guest.startCoop(0,fullSave());
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
    CHECK(guest.camera==host.teammate().camera && guest.player.hp==host.teammate().player.hp);
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
void runCoopTests() {
    synchronizedClock();
    handshake();
    sharedSimulation();
}
