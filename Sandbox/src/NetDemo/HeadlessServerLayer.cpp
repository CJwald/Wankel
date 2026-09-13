#include "HeadlessServerLayer.h"

#include <Wankel/ECS/Components.h>
#include <Wankel/Networking/NetChannel.h>
#include <Wankel/Networking/NetSerialize.h>
#include <Wankel/Networking/Replication/ReplicationComponents.h>

#include <cmath>

namespace Wankel {

namespace {

constexpr uint16_t HeadlessServerPort = 54322;
constexpr uint32_t HeadlessCubeNetworkId = 1;

struct PingBody {
    uint32_t Counter = 0;
    float SentAtSeconds = 0.0f;
};

template <typename S>
void serialize(S& s, PingBody& p) {
    s.value4b(p.Counter);
    s.value4b(p.SentAtSeconds);
}

using PongBody = PingBody;

} // namespace

HeadlessServerLayer::HeadlessServerLayer() : Layer("HeadlessServer") {
    m_StartTime = std::chrono::steady_clock::now();
    m_LastFrameTime = m_StartTime;

    m_Bus.RegisterHandler(Networking::NetMessageType::Ping, [this](Networking::NetPeer peer, const Networking::NetMessage& msg) {
        auto ping = Networking::DecodeBody<PingBody>(msg.Payload);
        if (!ping)
            return;
        WK_SERVER_INFO("HeadlessServer: ping #{0} from peer {1}, replying pong", ping->Counter, peer.Id);
        Networking::NetMessage pong = Networking::MakeMessage(Networking::NetMessageType::Pong, msg.Tick, PongBody {*ping});
        m_Host.Send(peer, pong, Networking::NetChannel::Reliable, true);
    });

    m_Host.SetOnMessage([this](Networking::NetPeer peer, const uint8_t* data, size_t len, uint8_t) { m_Bus.Dispatch(peer, data, len); });
    m_Host.SetOnConnect([](Networking::NetPeer peer) { WK_SERVER_INFO("HeadlessServer: peer {0} connected", peer.Id); });
    m_Host.SetOnDisconnect(
        [](Networking::NetPeer peer, uint32_t) { WK_SERVER_INFO("HeadlessServer: peer {0} disconnected", peer.Id); });

    Networking::NetHostConfig config;
    config.Mode = Networking::NetHostMode::Server;
    config.Port = HeadlessServerPort;
    config.MaxPeers = 8;
    if (!m_Host.Init(config)) {
        WK_SERVER_ERROR("HeadlessServer: failed to initialize on port {0}", HeadlessServerPort);
        return;
    }

    entt::entity cube = m_Registry.create();
    m_Registry.emplace<Networking::NetworkId>(cube, Networking::NetworkId {HeadlessCubeNetworkId, false});
    m_Registry.emplace<Transform>(cube);

    WK_SERVER_INFO("HeadlessServer: listening on port {0} (headless - no window, no renderer, no audio)",
                    HeadlessServerPort);
}

void HeadlessServerLayer::OnUpdate() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_LastFrameTime).count();
    m_LastFrameTime = now;
    m_ElapsedTime += dt;
    m_FrameCount++;

    for (auto e : m_Registry.view<Networking::NetworkId, Transform>()) {
        Transform& transform = m_Registry.get<Transform>(e);
        transform.LocalPosition.x = std::sin(m_ElapsedTime) * 5.0f;
    }

    m_ReplicationTimer += dt;
    if (m_ReplicationTimer >= (1.0f / 20.0f)) {
        m_ReplicationTimer = 0.0f;
        m_Tick++;
        m_ReplicationSystem.SnapshotAndSend(m_Registry, m_Host, m_Tick);
    }

    m_HeartbeatTimer += dt;
    if (m_HeartbeatTimer >= 1.0f) {
        m_HeartbeatTimer = 0.0f;
        float uptime = std::chrono::duration<float>(now - m_StartTime).count();
        float achievedHz = uptime > 0.0f ? (float)m_FrameCount / uptime : 0.0f;
        WK_SERVER_INFO("HeadlessServer: heartbeat - uptime {0:.1f}s, {1} ticks ({2:.1f} Hz achieved)", uptime,
                        m_FrameCount, achievedHz);
    }
}

} // namespace Wankel
