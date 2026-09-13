#pragma once

// Proves ApplicationSpecification::Headless actually works standalone: no Window/Renderer/
// AudioSystem/gamepad input gets constructed, Run() paces itself against TargetTickRate instead of
// a vsync wait, and a real NetHost (server mode) + ReplicationSystem run correctly in that
// environment - exactly the shape a future Mechtrix dedicated server needs. This is the risk
// NetDemoLayer's windowed, single-process ping/pong + replication demo does NOT cover (both sides
// there run inside one normal, non-headless Application).
//
// Run this alongside a client that connects to HeadlessServerPort (see HeadlessServerLayer.cpp) to
// see it accept a real connection with no window on this side at all.

#include <Wankel/Core/Layer.h>
#include <Wankel/Networking/NetDispatch.h>
#include <Wankel/Networking/NetHost.h>
#include <Wankel/Networking/Replication/ReplicationSystem.h>

#include <entt/entt.hpp>

#include <chrono>
#include <cstdint>

namespace Wankel {

class HeadlessServerLayer : public Layer {
public:
    HeadlessServerLayer();
    void OnUpdate() override;

private:
    Networking::NetHost m_Host;
    Networking::NetMessageBus m_Bus;
    Networking::ReplicationSystem m_ReplicationSystem;
    entt::registry m_Registry;

    std::chrono::steady_clock::time_point m_StartTime;
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_ElapsedTime = 0.0f;
    float m_ReplicationTimer = 0.0f;
    float m_HeartbeatTimer = 0.0f;
    uint32_t m_Tick = 0;
    uint64_t m_FrameCount = 0;
};

} // namespace Wankel
