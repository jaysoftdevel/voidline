#include "net/session.hpp"
#include "net/socket_ops.hpp"
#include <algorithm>
#include <utility>

namespace vl::net {
namespace {
constexpr std::string_view prefix="VL10-";
constexpr uint8_t heartbeat=0;
using namespace std::chrono_literals;

void appendHex(std::string& code,uint8_t byte) {
    constexpr char digits[]="0123456789ABCDEF";
    code+=digits[byte>>4];
    code+=digits[byte&15];
}
int hexDigit(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}
bool usable(in_addr addr) {
    const uint32_t host=ntohl(addr.s_addr);
    return host!=0 && host!=0xffffffff && (host>>28)<14;
}
}

std::string Session::localAddress() { return discoverAddress(); }

bool Session::host(const std::string& address,uint16_t port) {
    close();
    if (!socketRuntime()) { fail("Could not initialize networking."); return false; }
    in_addr advertised{};
    if (inet_pton(AF_INET,address.c_str(),&advertised)!=1 || !usable(advertised)) {
        fail("Enter a valid IPv4 address for this computer."); return false;
    }
    if (!randomBytes(token)) {
        fail("Could not generate the session key."); return false;
    }
    server=openTcp();
    sockaddr_in endpoint{};
    endpoint.sin_family=AF_INET; endpoint.sin_port=htons(port);
    if (server==invalidSocket || !listenExclusive(server) ||
        bind(server,reinterpret_cast<const sockaddr*>(&endpoint),sizeof(endpoint))<0 || listen(server,4)<0) {
        fail("Could not open the multiplayer port. Check whether another host is running."); return false;
    }
    SocketSize boundSize=sizeof(endpoint);
    if (getsockname(server,reinterpret_cast<sockaddr*>(&endpoint),&boundSize)<0) {
        fail("Could not determine the multiplayer port."); return false;
    }
    const uint16_t boundPort=ntohs(endpoint.sin_port);
    code=prefix;
    const auto* ip=reinterpret_cast<const uint8_t*>(&advertised.s_addr);
    for (size_t i=0;i<4;++i) appendHex(code,ip[i]);
    appendHex(code,boundPort>>8); appendHex(code,boundPort&255);
    for (const auto byte:token) appendHex(code,byte);
    status=State::waiting; hostSide=true;
    lan.advertise(boundPort);
    return true;
}

bool Session::join(const std::string& key) {
    close();
    if (!socketRuntime()) { fail("Could not initialize networking."); return false; }
    if (key.starts_with("VL1-") || key.starts_with("VL2-") || key.starts_with("VL3-") || key.starts_with("VL4-") || key.starts_with("VL5-") || key.starts_with("VL6-") || key.starts_with("VL7-") || key.starts_with("VL8-") || key.starts_with("VL9-")) {
        fail("Both players must run VOIDLINE 2.10.2 for matching doors and level layouts."); return false;
    }
    if (!key.starts_with(prefix) || key.size()!=prefix.size()+2*(6+token.size())) {
        fail("Enter the complete session key from the host."); return false;
    }
    std::array<uint8_t,6+tokenSize> decoded{};
    for (size_t i=0;i<decoded.size();++i) {
        const int hi=hexDigit(key[prefix.size()+2*i]),lo=hexDigit(key[prefix.size()+2*i+1]);
        if (hi<0 || lo<0) { fail("The session key contains an invalid character."); return false; }
        decoded[i]=uint8_t(16*hi+lo);
    }
    std::copy(decoded.begin()+6,decoded.end(),token.begin());
    sockaddr_in endpoint{};
    endpoint.sin_family=AF_INET; endpoint.sin_port=htons(uint16_t(256*decoded[4]+decoded[5]));
    std::copy_n(decoded.begin(),4,reinterpret_cast<uint8_t*>(&endpoint.sin_addr.s_addr));
    if (!usable(endpoint.sin_addr) || endpoint.sin_port==0) { fail("The key has an invalid network address."); return false; }
    return connectTo(endpoint,false);
}

bool Session::joinLan(const LanHost& host) {
    close();
    if (!socketRuntime()) { fail("Could not initialize networking."); return false; }
    sockaddr_in address{};
    address.sin_family=AF_INET;
    address.sin_port=htons(host.port);
    if (!host.port || inet_pton(AF_INET,host.address.c_str(),&address.sin_addr)!=1 ||
        !usable(address.sin_addr)) { fail("The LAN host address is invalid."); return false; }
    return connectTo(address,true);
}

bool Session::connectTo(const sockaddr_in& address,bool lanRequest) {
    lanJoin=lanRequest;
    peer=openTcp();
    if (peer==invalidSocket) { fail("Could not open a network connection."); return false; }
    status=State::connecting; heard=Clock::now();
    const int result=connect(peer,reinterpret_cast<const sockaddr*>(&address),sizeof(address));
    if (result==0) readySocket();
    else if (!connectPending()) { fail("Could not reach the host."); return false; }
    return true;
}

void Session::approve(bool allow) {
    if (!pendingApproval()) return;
    if (allow) {
        if (!send(Packet::welcome)) return;
        // Keep the TCP listener alive to reject excess connections immediately.
        // Closing it here left Windows clients waiting for TCP connect/handshake
        // timeouts even though the two-player session was already full.
        lan.close(); // A full mission should no longer appear in LAN discovery.
        status=State::connected;
    } else {
        if (!send(Packet::denied)) { dropCandidate(); return; }
        status=State::rejecting;
        flush();
        if (status==State::rejecting && !pendingOutput()) dropCandidate();
    }
}

void Session::readySocket() {
    lowLatency(peer);
    status=State::handshake; heard=sent=Clock::now();
    if (!hostSide) {
        if (lanJoin) {
            constexpr std::array<uint8_t,4> lanProtocol{'V','L','1','0'};
            send(Packet::helloLan,lanProtocol);
        } else send(Packet::hello,token);
        status=State::approval;
    }
}

void Session::dropCandidate() {
    closeSocket(peer);
    incoming.clear(); outgoing.clear(); packets.clear(); sentBytes=0;
    candidateAddress.clear();
    status=State::waiting;
}
void Session::close() {
    dropCandidate(); closeSocket(server); lan.close();
    hostSide=lanJoin=false; status=State::idle; code.clear(); issue.clear();
}
void Session::fail(std::string message) {
    close(); issue=std::move(message);
}

bool Session::send(Packet packet,std::span<const uint8_t> data) {
    if (peer==invalidSocket || data.size()+1>maxPacket) return false;
    if (sentBytes) {
        outgoing.erase(outgoing.begin(),outgoing.begin()+sentBytes);
        sentBytes=0;
    }
    if (outgoing.size()+data.size()+5>maxQueue) {
        fail("The connection is too slow to keep the mission synchronized."); return false;
    }
    const auto size=static_cast<uint32_t>(data.size()+1);
    for (int shift=24;shift>=0;shift-=8) outgoing.push_back(uint8_t(size>>shift));
    outgoing.push_back(static_cast<uint8_t>(packet));
    outgoing.insert(outgoing.end(),data.begin(),data.end());
    return true;
}

bool Session::process(std::span<const uint8_t> data) {
    if (status==State::handshake && hostSide) {
        const bool keyed=data[0]==static_cast<uint8_t>(Packet::hello) &&
            data.size()==token.size()+1 &&
            std::equal(token.begin(),token.end(),data.begin()+1);
        const bool discovered=data.size()==5 && data[0]==static_cast<uint8_t>(Packet::helloLan) &&
            data[1]=='V' && data[2]=='L' && data[3]=='1' && data[4]=='0';
        if (!keyed && !discovered) { dropCandidate(); return false; }
        status=State::approval;
        heard=Clock::now();
        return true;
    }
    if (status==State::approval && !hostSide) {
        if (data.size()==1 && data[0]==static_cast<uint8_t>(Packet::denied)) {
            fail("The host declined your request to join."); return false;
        }
        if (data.size()!=1 || data[0]!=static_cast<uint8_t>(Packet::welcome)) {
            fail("The host sent an incompatible approval response."); return false;
        }
        status=State::connected;
        heard=Clock::now();
        return true;
    }
    if (status==State::handshake || status==State::approval || status==State::rejecting) {
        fail("Received a message before the session was approved."); return false;
    }
    if (data.size()==1 && data[0]==heartbeat) return true;
    if (data[0]<static_cast<uint8_t>(Packet::ready) || data[0]>static_cast<uint8_t>(Packet::lobbyMode) || packets.size()>=128) {
        fail("Received an invalid multiplayer message."); return false;
    }
    packets.emplace_back(data.begin(),data.end());
    return true;
}

void Session::flush() {
    while (sentBytes<outgoing.size()) {
        const auto n=writeSocket(peer,std::span(outgoing).subspan(sentBytes));
        if (n<0) {
            const auto error=lastIoStatus();
            if (error==IoStatus::interrupted) continue;
            if (error==IoStatus::again) return;
        }
        if (n<=0) { fail("The other player disconnected."); return; }
        sentBytes+=static_cast<size_t>(n); sent=Clock::now();
    }
    outgoing.clear(); sentBytes=0;
}

void Session::poll() {
    lan.poll();
    if (hostSide && status==State::connected && server!=invalidSocket) {
        // Consume pending connects rather than leaving them in the OS backlog.
        // On Windows a connect against a recently closed listener may linger,
        // making the joining client time out instead of rejecting promptly.
        for (int attempts=0; attempts<8; ++attempts) {
            Socket extra=acceptPeer(server);
            if (extra==invalidSocket) break;
            constexpr std::array<uint8_t,5> deniedFrame{0,0,0,1,static_cast<uint8_t>(Packet::denied)};
            (void)writeSocket(extra,deniedFrame);
            closeSocket(extra);
        }
    }
    if (status==State::waiting) {
        peer=acceptPeer(server);
        if (peer!=invalidSocket) {
            sockaddr_in address{};
            SocketSize length=sizeof(address);
            if (getpeername(peer,reinterpret_cast<sockaddr*>(&address),&length)==0) {
                char ip[INET_ADDRSTRLEN]{};
                if (inet_ntop(AF_INET,&address.sin_addr,ip,sizeof(ip))) candidateAddress=ip;
            }
            readySocket();
        }
    }
    if (peer==invalidSocket) return;
    const auto now=Clock::now();
    if (status==State::connecting) {
        const auto connection=checkConnection(peer);
        if (connection==ConnectStatus::waiting) {
            if (now-heard>8s) fail("Connection timed out. Check that the host is reachable.");
            return;
        }
        if (connection==ConnectStatus::failed) {
            fail("Could not reach the host. Check the session key and network."); return;
        }
        readySocket();
    }
    if (now-heard>((status==State::handshake)?5s:(status==State::approval?30s:10s))) {
        if (hostSide && (status==State::handshake || status==State::approval)) dropCandidate();
        else fail("The multiplayer connection timed out.");
        return;
    }
    size_t consumed=0;
    for (int reads=0;reads<32;++reads) {
        uint8_t buffer[8192];
        const auto n=readSocket(peer,buffer);
        if (n<0) {
            const auto error=lastIoStatus();
            if (error==IoStatus::interrupted) continue;
            if (error==IoStatus::again) break;
        }
        if (n<=0) {
            if (hostSide && (status==State::handshake || status==State::approval || status==State::rejecting)) dropCandidate();
            else fail(status==State::handshake?"The host did not accept this key.":"The other player disconnected.");
            return;
        }
        if (status==State::connected) heard=Clock::now();
        incoming.insert(incoming.end(),buffer,buffer+n);
        while (incoming.size()-consumed>=4) {
            const auto* header=incoming.data()+consumed;
            const auto size=(uint32_t(header[0])<<24)|(uint32_t(header[1])<<16)|(uint32_t(header[2])<<8)|header[3];
            if (!size || size>maxPacket) {
                if (hostSide && (status==State::handshake || status==State::approval || status==State::rejecting)) dropCandidate();
                else fail("Received an invalid multiplayer message.");
                return;
            }
            if (incoming.size()-consumed<4+size) break;
            if (!process(std::span(header+4,size))) return;
            consumed+=4+size;
        }
    }
    incoming.erase(incoming.begin(),incoming.begin()+consumed);
    if (status==State::connected && now-sent>2s && outgoing.empty()) send(static_cast<Packet>(heartbeat));
    flush();
    if (status==State::rejecting && !pendingOutput()) dropCandidate();
}
std::vector<Bytes> Session::receive() { return std::exchange(packets,{}); }
}
