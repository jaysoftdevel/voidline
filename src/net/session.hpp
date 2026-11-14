#pragma once
#include "net/socket.hpp"
#include "net/lan.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace vl::net {
using Bytes=std::vector<uint8_t>;
enum class Packet : uint8_t {hello=1,welcome,ready,prepare,controls,world,clockProbe,clockReply,loaded,schedule,scheduled,epilogue,epilogueDone,helloLan=14,denied=15,operative=16,skipRequest=17,skipOpening=18};

class Session {
public:
    enum class State {idle,waiting,connecting,handshake,approval,rejecting,connected};
    ~Session() { close(); }
    Session()=default;
    Session(const Session&)=delete;
    Session& operator=(const Session&)=delete;
    bool host(const std::string& address,uint16_t port=38274);
    bool join(const std::string& key);
    bool joinLan(const LanHost& host);
    bool searchLan() { return lan.search(); }
    const std::vector<LanHost>& nearby() const { return lan.hosts(); }
    bool discoverable() const { return lan.advertising(); }
    bool pendingApproval() const { return hostSide && status==State::approval; }
    const std::string& joiningAddress() const { return candidateAddress; }
    void approve(bool allow);
    void poll();
    void close();
    void fail(std::string message);
    bool send(Packet packet,std::span<const uint8_t> data={});
    std::vector<Bytes> receive();
    void flush();
    bool pendingOutput() const { return sentBytes<outgoing.size(); }
    const std::string& key() const { return code; }
    const std::string& error() const { return issue; }
    State state() const { return status; }
    bool hosting() const { return server!=invalidSocket || hostSide; }
    static std::string localAddress();
private:
    using Clock=std::chrono::steady_clock;
    static constexpr size_t tokenSize=12;
    static constexpr size_t maxPacket=256*1024,maxQueue=2*maxPacket;
    std::array<uint8_t,tokenSize> token{};
    Socket server=invalidSocket,peer=invalidSocket;
    bool hostSide=false,lanJoin=false;
    LanDiscovery lan;
    std::string candidateAddress;
    State status=State::idle;
    Clock::time_point heard{},sent{};
    std::string code,issue;
    Bytes incoming,outgoing;
    size_t sentBytes=0;
    std::vector<Bytes> packets;
    void dropCandidate();
    bool process(std::span<const uint8_t> data);
    void readySocket();
    bool connectTo(const sockaddr_in& address,bool lanRequest);
};
}
