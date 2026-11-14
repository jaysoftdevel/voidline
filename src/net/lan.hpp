#pragma once
#include "net/socket.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace vl::net {
// UDP discovery is an invitation to request approval, never an authentication token.
// TCP gameplay remains on the session port; no gameplay datagrams are broadcast.
struct LanHost {
    std::string name,address;
    uint16_t port=0;
};

class LanDiscovery {
public:
    ~LanDiscovery() { close(); }
    bool advertise(uint16_t port);
    bool search();
    void poll();
    void close();
    bool advertising() const { return listener!=invalidSocket; }
    bool searching() const { return scanner!=invalidSocket; }
    const std::vector<LanHost>& hosts() const { return found; }
private:
    using Clock=std::chrono::steady_clock;
    static constexpr uint16_t discoveryPort=38275;
    Socket listener=invalidSocket,scanner=invalidSocket;
    uint16_t gamePort=0;
    std::string hostname;
    std::vector<LanHost> found;
    Clock::time_point lastQuery{},scanStarted{};
    void broadcast();
};
}
