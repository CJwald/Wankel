#pragma once

// Wankel-only networking hooks verification (see docs/plan for the phase this belongs to): spins up
// an in-process ENet server + client over loopback and exercises, end to end:
//   1. NetHost connect/send/receive + NetMessageBus main-thread dispatch, via a ping/pong exchange.
//   2. ReplicationSystem: a server-side cube driven by a sine wave, replicated to a client-side
//      shadow entity purely through NetworkId + the engine's Transform replication row.
// Gated behind WANKEL_NET_DEMO (off by default) so it never affects the normal flying-ship Sandbox
// build - see Sandbox/CMakeLists.txt.

#include <Wankel/Core/Layer.h>
#include <Wankel/Networking/NetDispatch.h>
#include <Wankel/Networking/NetHost.h>
#include <Wankel/Networking/Replication/ReplicationSystem.h>

#include <entt/entt.hpp>

#include <cstdint>

namespace Wankel {

class NetDemoLayer : public Layer {
public:
    NetDemoLayer();
    ~NetDemoLayer() override;

    void OnUpdate() override;

private:
    void SetupServer();
    void SetupClient();
    void LogReplicationStatus();

    Networking::NetHost m_ServerHost;
    Networking::NetHost m_ClientHost;
    Networking::NetMessageBus m_ServerBus;
    Networking::NetMessageBus m_ClientBus;
    Networking::ReplicationSystem m_ReplicationSystem;

    entt::registry m_ServerRegistry;
    entt::registry m_ClientRegistry;

    Networking::NetPeer m_ClientSeenServer; // set once the client's connection to the server completes
    bool m_ClientConnected = false;

    float m_LastFrameTime = 0.0f;
    float m_ElapsedTime = 0.0f;
    float m_PingTimer = 0.0f;
    float m_ReplicationTimer = 0.0f;
    float m_StatusLogTimer = 0.0f;
    uint32_t m_PingCounter = 0;
    uint32_t m_Tick = 0;
};

} // namespace Wankel
