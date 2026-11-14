#include "net/socket_ops.hpp"
#include <string_view>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <iphlpapi.h>
#include <vector>
#else
#include <cerrno>
#include <ifaddrs.h>
#include <net/if.h>
#include <poll.h>
#include <sys/random.h>
#include <unistd.h>
#endif

namespace vl::net {
#ifdef _WIN32
namespace {
bool nonblocking(Socket socket) {
    u_long on=1;
    return ioctlsocket(socket,FIONBIO,&on)==0;
}
struct Winsock {
    WSADATA data{};
    bool ready=WSAStartup(MAKEWORD(2,2),&data)==0;
    ~Winsock() { if (ready) WSACleanup(); }
};
}
#endif

bool socketRuntime() {
#ifdef _WIN32
    static const Winsock runtime;
    return runtime.ready;
#else
    return true;
#endif
}

Socket openTcp() {
#ifdef _WIN32
    Socket socket=WSASocketW(AF_INET,SOCK_STREAM,IPPROTO_TCP,nullptr,0,WSA_FLAG_NO_HANDLE_INHERIT);
    if (socket!=invalidSocket && !nonblocking(socket)) closeSocket(socket);
    return socket;
#else
    return ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
#endif
}

Socket openUdp() {
#ifdef _WIN32
    Socket socket=WSASocketW(AF_INET,SOCK_DGRAM,IPPROTO_UDP,nullptr,0,WSA_FLAG_NO_HANDLE_INHERIT);
    if (socket!=invalidSocket && !nonblocking(socket)) closeSocket(socket);
    return socket;
#else
    return ::socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
#endif
}

void closeSocket(Socket& socket) {
    if (socket==invalidSocket) return;
#ifdef _WIN32
    closesocket(std::exchange(socket,invalidSocket));
#else
    ::close(std::exchange(socket,invalidSocket));
#endif
}

Socket acceptPeer(Socket server) {
#ifdef _WIN32
    Socket peer=accept(server,nullptr,nullptr);
    if (peer!=invalidSocket &&
        (!SetHandleInformation(reinterpret_cast<HANDLE>(peer),HANDLE_FLAG_INHERIT,0) || !nonblocking(peer)))
        closeSocket(peer);
    return peer;
#else
    return accept4(server,nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);
#endif
}

bool listenExclusive(Socket socket) {
    const int yes=1;
#ifdef _WIN32
    return setsockopt(socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&yes),sizeof(yes))==0;
#else
    return setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))==0;
#endif
}

void lowLatency(Socket socket) {
    const int yes=1;
    setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&yes),sizeof(yes));
    // Limit stale snapshots waiting inside the OS when a peer is slow.
    const int sendBuffer=64*1024;
    setsockopt(socket,SOL_SOCKET,SO_SNDBUF,reinterpret_cast<const char*>(&sendBuffer),sizeof(sendBuffer));
}

bool connectPending() {
#ifdef _WIN32
    const int error=WSAGetLastError();
    return error==WSAEWOULDBLOCK || error==WSAEINPROGRESS;
#else
    return errno==EINPROGRESS;
#endif
}

ConnectStatus checkConnection(Socket socket) {
#ifdef _WIN32
    // Failed nonblocking connects are reported through exceptfds by Winsock.
    fd_set writable,exception;
    FD_ZERO(&writable); FD_SET(socket,&writable);
    FD_ZERO(&exception); FD_SET(socket,&exception);
    timeval immediate{};
    const int ready=select(0,nullptr,&writable,&exception,&immediate);
#else
    pollfd fd{socket,POLLOUT,0};
    const int ready=poll(&fd,1,0);
#endif
    if (!ready || (ready<0 && lastIoStatus()==IoStatus::interrupted)) return ConnectStatus::waiting;
    if (ready<0) return ConnectStatus::failed;
    int error=0;
    SocketSize size=sizeof(error);
    return getsockopt(socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size)==0 && !error
        ?ConnectStatus::connected:ConnectStatus::failed;
}

int writeSocket(Socket socket,std::span<const uint8_t> bytes) {
#ifdef _WIN32
    return send(socket,reinterpret_cast<const char*>(bytes.data()),static_cast<int>(bytes.size()),0);
#else
    return static_cast<int>(send(socket,bytes.data(),bytes.size(),MSG_NOSIGNAL));
#endif
}
int readSocket(Socket socket,std::span<uint8_t> bytes) {
#ifdef _WIN32
    return recv(socket,reinterpret_cast<char*>(bytes.data()),static_cast<int>(bytes.size()),0);
#else
    return static_cast<int>(recv(socket,bytes.data(),bytes.size(),0));
#endif
}
IoStatus lastIoStatus() {
#ifdef _WIN32
    const int error=WSAGetLastError();
    if (error==WSAEWOULDBLOCK) return IoStatus::again;
    if (error==WSAEINTR) return IoStatus::interrupted;
#else
    if (errno==EAGAIN || errno==EWOULDBLOCK) return IoStatus::again;
    if (errno==EINTR) return IoStatus::interrupted;
#endif
    return IoStatus::failed;
}

bool randomBytes(std::span<uint8_t> bytes) {
#ifdef _WIN32
    return BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0;
#else
    while (!bytes.empty()) {
        const auto count=getrandom(bytes.data(),bytes.size(),0);
        if (count<0 && errno==EINTR) continue;
        if (count<=0) return false;
        bytes=bytes.subspan(static_cast<size_t>(count));
    }
    return true;
#endif
}

std::string discoverAddress() {
    std::string best="127.0.0.1";
    int rank=0;
    auto consider=[&](const sockaddr* socketAddress,int priority) {
        if (!socketAddress || socketAddress->sa_family!=AF_INET || priority<=rank) return;
        const auto& address=reinterpret_cast<const sockaddr_in*>(socketAddress)->sin_addr;
        const auto host=ntohl(address.s_addr);
        if (!host || (host>>24)==127 || (host>>16)==0xa9fe || (host>>28)>=14) return;
        char buffer[INET_ADDRSTRLEN]{};
        if (inet_ntop(AF_INET,&address,buffer,sizeof(buffer))) { best=buffer; rank=priority; }
    };
#ifdef _WIN32
    if (!socketRuntime()) return best;
    ULONG size=16384;
    std::vector<uint8_t> buffer(size);
    for (int attempt=0;attempt<3;++attempt) {
        const auto first=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        const auto result=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|
                                               GAA_FLAG_SKIP_DNS_SERVER,nullptr,first,&size);
        if (result==ERROR_BUFFER_OVERFLOW) { buffer.resize(size); continue; }
        if (result!=NO_ERROR) return best;
        for (auto* adapter=first;adapter;adapter=adapter->Next) {
            if (adapter->OperStatus!=IfOperStatusUp || adapter->IfType==IF_TYPE_SOFTWARE_LOOPBACK) continue;
            const int priority=adapter->IfType==IF_TYPE_ETHERNET_CSMACD || adapter->IfType==IF_TYPE_IEEE80211?2:1;
            for (auto* address=adapter->FirstUnicastAddress;address;address=address->Next)
                consider(address->Address.lpSockaddr,priority);
        }
        break;
    }
#else
    ifaddrs* addresses=nullptr;
    if (getifaddrs(&addresses)!=0) return best;
    for (auto* it=addresses;it;it=it->ifa_next) {
        if (!(it->ifa_flags&IFF_UP) || (it->ifa_flags&IFF_LOOPBACK)) continue;
        const std::string_view name=it->ifa_name;
        consider(it->ifa_addr,name.starts_with("en") || name.starts_with("wl") || name.starts_with("eth")?2:1);
    }
    freeifaddrs(addresses);
#endif
    return best;
}
}
