#include "net/lan.hpp"
#include "net/socket_ops.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#include <unistd.h>
#endif

namespace vl::net {
namespace {
constexpr std::array<uint8_t,5> query{'V','L','D','9','?'};
constexpr std::array<uint8_t,5> answer{'V','L','D','9','!'};
using namespace std::chrono_literals;

sockaddr_in endpoint(uint32_t address,uint16_t port) {
    sockaddr_in result{};
    result.sin_family=AF_INET;
    result.sin_port=htons(port);
    result.sin_addr.s_addr=htonl(address);
    return result;
}
bool transmit(Socket s,std::span<const uint8_t> msg,const sockaddr_in& to) {
#ifdef _WIN32
    return sendto(s,reinterpret_cast<const char*>(msg.data()),static_cast<int>(msg.size()),0,
                  reinterpret_cast<const sockaddr*>(&to),sizeof(to))==static_cast<int>(msg.size());
#else
    return sendto(s,msg.data(),msg.size(),0,reinterpret_cast<const sockaddr*>(&to),sizeof(to))==static_cast<int>(msg.size());
#endif
}
int receive(Socket s,std::span<uint8_t> msg,sockaddr_in& from) {
    SocketSize len=sizeof(from);
#ifdef _WIN32
    return recvfrom(s,reinterpret_cast<char*>(msg.data()),static_cast<int>(msg.size()),0,
                    reinterpret_cast<sockaddr*>(&from),&len);
#else
    return static_cast<int>(recvfrom(s,msg.data(),msg.size(),0,reinterpret_cast<sockaddr*>(&from),&len));
#endif
}
std::string ipv4(const sockaddr_in& from) {
    char text[INET_ADDRSTRLEN]{};
    if (!inet_ntop(AF_INET,&from.sin_addr,text,sizeof(text))) return {};
    return text;
}
}

void LanDiscovery::close() {
    closeSocket(listener); closeSocket(scanner);
    found.clear(); gamePort=0; hostname.clear();
    lastQuery={}; scanStarted={};
}

bool LanDiscovery::advertise(uint16_t port) {
    close();
    if (!socketRuntime() || port==0) return false;
    listener=openUdp();
    if (listener==invalidSocket) return false;
    const auto address=endpoint(INADDR_ANY,discoveryPort);
    if (bind(listener,reinterpret_cast<const sockaddr*>(&address),sizeof(address))<0) {
        closeSocket(listener); return false;
    }
    gamePort=port;
    char name[64]{};
    if (gethostname(name,sizeof(name)-1)!=0) std::strcpy(name,"VOIDLINE HOST");
    for (const unsigned char c:std::string_view(name)) {
        if (hostname.size()==25) break;
        if (c>=32 && c<127) hostname+=static_cast<char>(c);
    }
    if (hostname.empty()) hostname="VOIDLINE HOST";
    return true;
}

bool LanDiscovery::search() {
    close();
    if (!socketRuntime()) return false;
    scanner=openUdp();
    if (scanner==invalidSocket) return false;
    const int enabled=1;
    if (setsockopt(scanner,SOL_SOCKET,SO_BROADCAST,reinterpret_cast<const char*>(&enabled),sizeof(enabled))<0) {
        closeSocket(scanner); return false;
    }
    lastQuery={}; scanStarted=Clock::now();
    broadcast();
    return true;
}

void LanDiscovery::broadcast() {
    if (scanner==invalidSocket) return;
    // Broadcast on the subnet and loopback for local testing/hosts.
    transmit(scanner,query,endpoint(INADDR_BROADCAST,discoveryPort));
    transmit(scanner,query,endpoint(INADDR_LOOPBACK,discoveryPort));
    lastQuery=Clock::now();
}

void LanDiscovery::poll() {
    if (scanner!=invalidSocket && Clock::now()-lastQuery>=1100ms && Clock::now()-scanStarted<7s)
        broadcast();
    if (listener!=invalidSocket) {
        for (int i=0;i<24;++i) {
            std::array<uint8_t,96> buf{};
            sockaddr_in from{};
            const int count=receive(listener,buf,from);
            if (count<0) {
                if (lastIoStatus()==IoStatus::interrupted) continue;
                break;
            }
            if (count!=static_cast<int>(query.size()) ||
                !std::equal(query.begin(),query.end(),buf.begin())) continue;
            std::array<uint8_t,5+2+1+25> reply{};
            std::copy(answer.begin(),answer.end(),reply.begin());
            reply[5]=static_cast<uint8_t>(gamePort>>8);
            reply[6]=static_cast<uint8_t>(gamePort&255);
            reply[7]=static_cast<uint8_t>(hostname.size());
            std::copy(hostname.begin(),hostname.end(),reply.begin()+8);
            transmit(listener,std::span(reply).first(8+hostname.size()),from);
        }
    }
    if (scanner!=invalidSocket) {
        for (int i=0;i<32;++i) {
            std::array<uint8_t,96> buf{};
            sockaddr_in from{};
            const int count=receive(scanner,buf,from);
            if (count<0) {
                if (lastIoStatus()==IoStatus::interrupted) continue;
                break;
            }
            if (count<8 || count>33 || !std::equal(answer.begin(),answer.end(),buf.begin()) ||
                count!=8+buf[7] || !buf[7]) continue;
            const uint16_t port=uint16_t((uint16_t(buf[5])<<8)|buf[6]);
            if (!port) continue;
            const auto ip=ipv4(from);
            if (ip.empty()) continue;
            std::string name;
            for (int j=8;j<count;++j)
                if (buf[j]>=32 && buf[j]<127) name.push_back(static_cast<char>(buf[j]));
            if (name.empty()) name="VOIDLINE HOST";
            const auto it=std::find_if(found.begin(),found.end(),[&](const LanHost& h) {
                return h.address==ip && h.port==port;
            });
            if (it!=found.end()) it->name=name;
            else if (found.size()<8) found.push_back({name,ip,port});
        }
    }
}
}
