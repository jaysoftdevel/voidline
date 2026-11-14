#pragma once
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

namespace vl::net {
#ifdef _WIN32
using Socket=SOCKET;
using SocketSize=int;
inline constexpr Socket invalidSocket=INVALID_SOCKET;
#else
using Socket=int;
using SocketSize=socklen_t;
inline constexpr Socket invalidSocket=-1;
#endif
}
