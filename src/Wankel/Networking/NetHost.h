#pragma once

#include "NetChannel.h"
#include "NetMessage.h"
#include "NetPeer.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Wankel::Networking {

enum class NetHostMode { Client, Server };

struct NetHostConfig {
    NetHostMode Mode = NetHostMode::Client;
    uint16_t Port = 0;      // required for Server, ignored (ephemeral) for Client
    size_t MaxPeers = 128;  // server only - sized for the "dozens-~100 players" target, not a cap
};

// Thin wrapper over a single ENet host (client or server - ENet itself doesn't distinguish the two
// beyond whether the host was created bound to a listen address). Not a singleton, unlike
// Application/Renderer: a client needs exactly one, and so will a future dedicated server, with no
// shared global state to arbitrate between them.
//
// PollEvents runs on a dedicated thread owned by this object (started by Init, stopped by
// Shutdown) rather than via JobSystem::Submit, since JobSystem's worker pool is fire-and-forget
// one-shot work, not a home for a perpetual loop. Callbacks (OnConnect/OnDisconnect/OnMessage) are
// invoked FROM THAT THREAD - they must not touch the ECS registry, OpenGL, or any other
// main-thread-only state directly. NetMessageBus (NetDispatch.h) is the seam that marshals
// incoming messages onto the main thread via JobSystem::SubmitMainThread before handling them.
class NetHost {
public:
    NetHost();
    ~NetHost();

    NetHost(const NetHost&) = delete;
    NetHost& operator=(const NetHost&) = delete;

    bool Init(const NetHostConfig& config);
    void Shutdown();

    // Client-only: begins connecting: asynchronous - the resulting PeerId is delivered via the
    // OnConnect callback once ENet completes the handshake, not returned here.
    void Connect(const std::string& host, uint16_t port);

    void Send(NetPeer peer, const NetMessage& msg, NetChannel channel, bool reliable);
    void Broadcast(const NetMessage& msg, NetChannel channel, bool reliable); // server only
    void Disconnect(NetPeer peer, uint32_t reasonCode = 0);

    using ConnectCallback = std::function<void(NetPeer)>;
    using DisconnectCallback = std::function<void(NetPeer, uint32_t reason)>;
    using MessageCallback = std::function<void(NetPeer, const uint8_t* data, size_t len, uint8_t channel)>;

    // Must be called before Init() - the poll thread starts inside Init() and callbacks may fire
    // as soon as it does.
    void SetOnConnect(ConnectCallback cb);
    void SetOnDisconnect(DisconnectCallback cb);
    void SetOnMessage(MessageCallback cb);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace Wankel::Networking
