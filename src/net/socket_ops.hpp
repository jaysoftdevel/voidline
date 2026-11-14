#pragma once
#include "net/socket.hpp"
#include <span>
#include <string>

namespace vl::net {
enum class IoStatus {again,interrupted,failed};
enum class ConnectStatus {waiting,connected,failed};

bool socketRuntime();
Socket openTcp();
Socket openUdp();
void closeSocket(Socket& socket);
Socket acceptPeer(Socket server);
bool listenExclusive(Socket socket);
void lowLatency(Socket socket);
bool connectPending();
ConnectStatus checkConnection(Socket socket);
int writeSocket(Socket socket,std::span<const uint8_t> bytes);
int readSocket(Socket socket,std::span<uint8_t> bytes);
IoStatus lastIoStatus();
bool randomBytes(std::span<uint8_t> bytes);
std::string discoverAddress();
}
