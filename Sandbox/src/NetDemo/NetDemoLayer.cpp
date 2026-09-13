#include "NetDemoLayer.h"

#include <Wankel/Core/JobSystem.h>
#include <Wankel/Core/Time.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Networking/NetChannel.h>
#include <Wankel/Networking/NetSerialize.h>
#include <Wankel/Networking/Replication/ReplicationComponents.h>

#include <cmath>

namespace Wankel {

namespace {

constexpr uint16_t DemoPort = 54321;
constexpr uint32_t DemoCubeNetworkId = 1;

struct PingBody {
    uint32_t Counter = 0;
    float SentAtSeconds = 0.0f;
};

template <typename S>
void serialize(S& s, PingBody& p) {
    s.value4b(p.Counter);
    s.value4b(p.SentAtSeconds);
}

using PongBody = PingBody; // Pong just echoes the ping back verbatim so the client can compute RTT.

entt::entity FindByNetworkId(entt::registry& registry, uint32_t id) {
    for (auto e : registry.view<Networking::NetworkId>()) {
        if (registry.get<Networking::NetworkId>(e).Id == id)
            return e;
    }
    return entt::null;
}

} // namespace

NetDemoLayer::NetDemoLayer() : Layer("NetDemo") {
    SetupServer();
    SetupClient();
}

NetDemoLayer::~NetDemoLayer() {
    m_ClientHost.Shutdown();
    m_ServerHost.Shutdown();
}

void NetDemoLayer::SetupServer() {
    m_ServerBus.RegisterHandler(Networking::NetMessageType::Ping, [this](Networking::NetPeer peer, const Networking::NetMessage& msg) {
        auto ping = Networking::DecodeBody<PingBody>(msg.Payload);
        if (!ping)
            return;
        WK_SERVER_INFO("NetDemo(server): ping #{0} from peer {1}, replying pong", ping->Counter, peer.Id);
        Networking::NetMessage pong = Networking::MakeMessage(Networking::NetMessageType::Pong, msg.Tick, PongBody {*ping});
        m_ServerHost.Send(peer, pong, Networking::NetChannel::Reliable, true);
    });

    m_ServerHost.SetOnMessage([this](Networking::NetPeer peer, const uint8_t* data, size_t len, uint8_t) {
        m_ServerBus.Dispatch(peer, data, len);
    });
    m_ServerHost.SetOnConnect(
        [](Networking::NetPeer peer) { WK_SERVER_INFO("NetDemo(server): peer {0} connected", peer.Id); });

    Networking::NetHostConfig config;
    config.Mode = Networking::NetHostMode::Server;
    config.Port = DemoPort;
    config.MaxPeers = 8;
    if (!m_ServerHost.Init(config)) {
        WK_SERVER_ERROR("NetDemo(server): failed to initialize");
        return;
    }

    // Server-owned cube: driven by a sine wave below, replicated purely through NetworkId +
    // ReplicationSystem's Transform row - the client never sees this entity's type, only its
    // networked position/orientation.
    entt::entity cube = m_ServerRegistry.create();
    m_ServerRegistry.emplace<Networking::NetworkId>(cube, Networking::NetworkId {DemoCubeNetworkId, false});
    m_ServerRegistry.emplace<Transform>(cube);
}

void NetDemoLayer::SetupClient() {
    m_ClientBus.RegisterHandler(Networking::NetMessageType::Pong, [this](Networking::NetPeer, const Networking::NetMessage& msg) {
        auto pong = Networking::DecodeBody<PongBody>(msg.Payload);
        if (!pong)
            return;
        float rttMs = (Time::GetTime() - pong->SentAtSeconds) * 1000.0f;
        WK_CLIENT_INFO("NetDemo(client): pong #{0} received, round-trip {1:.2f} ms", pong->Counter, rttMs);
    });
    m_ReplicationSystem.RegisterHandlers(m_ClientBus, m_ClientRegistry);

    m_ClientHost.SetOnMessage([this](Networking::NetPeer peer, const uint8_t* data, size_t len, uint8_t) {
        m_ClientBus.Dispatch(peer, data, len);
    });
    m_ClientHost.SetOnConnect([this](Networking::NetPeer peer) {
        WK_CLIENT_INFO("NetDemo(client): connected to server (peer id {0})", peer.Id);
        JobSystem::SubmitMainThread([this, peer] {
            m_ClientSeenServer = peer;
            m_ClientConnected = true;
        });
    });

    Networking::NetHostConfig config;
    config.Mode = Networking::NetHostMode::Client;
    if (!m_ClientHost.Init(config)) {
        WK_CLIENT_ERROR("NetDemo(client): failed to initialize");
        return;
    }

    m_ClientHost.Connect("127.0.0.1", DemoPort);
}

void NetDemoLayer::LogReplicationStatus() {
    entt::entity serverCube = FindByNetworkId(m_ServerRegistry, DemoCubeNetworkId);
    entt::entity clientShadow = FindByNetworkId(m_ClientRegistry, DemoCubeNetworkId);

    auto* serverTransform = serverCube == entt::null ? nullptr : m_ServerRegistry.try_get<Transform>(serverCube);
    auto* clientTransform = clientShadow == entt::null ? nullptr : m_ClientRegistry.try_get<Transform>(clientShadow);

    if (serverTransform && clientTransform) {
        WK_CLIENT_INFO("NetDemo: server cube x={0:.2f} | client shadow x={1:.2f}", serverTransform->LocalPosition.x,
                        clientTransform->LocalPosition.x);
    } else {
        WK_CLIENT_INFO("NetDemo: waiting for first replicated snapshot from server...");
    }
}

void NetDemoLayer::OnUpdate() {
    float time = Time::GetTime();
    float dt = time - m_LastFrameTime;
    m_LastFrameTime = time;
    m_ElapsedTime += dt;

    // Drive the server cube along a sine wave so the client-side shadow visibly has to track it.
    for (auto e : m_ServerRegistry.view<Networking::NetworkId, Transform>()) {
        Transform& transform = m_ServerRegistry.get<Transform>(e);
        transform.LocalPosition.x = std::sin(m_ElapsedTime) * 5.0f;
    }

    m_PingTimer += dt;
    if (m_ClientConnected && m_PingTimer >= 1.0f) {
        m_PingTimer = 0.0f;
        PingBody ping {m_PingCounter++, time};
        Networking::NetMessage msg = Networking::MakeMessage(Networking::NetMessageType::Ping, m_Tick, ping);
        WK_CLIENT_INFO("NetDemo(client): sending ping #{0}", ping.Counter);
        m_ClientHost.Send(m_ClientSeenServer, msg, Networking::NetChannel::Reliable, true);
    }

    m_ReplicationTimer += dt;
    if (m_ReplicationTimer >= (1.0f / 20.0f)) { // 20Hz replication tick, independent of render rate
        m_ReplicationTimer = 0.0f;
        m_Tick++;
        m_ReplicationSystem.SnapshotAndSend(m_ServerRegistry, m_ServerHost, m_Tick);
    }

    m_StatusLogTimer += dt;
    if (m_StatusLogTimer >= 1.0f) {
        m_StatusLogTimer = 0.0f;
        LogReplicationStatus();
    }
}

} // namespace Wankel
