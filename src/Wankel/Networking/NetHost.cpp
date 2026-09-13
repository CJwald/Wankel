#include "wkpch.h"

#include "NetHost.h"

// Single-header ENet - the implementation must be generated in exactly one translation unit.
#define ENET_IMPLEMENTATION
#include <enet.h>

#include "NetSerialize.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Wankel::Networking {

struct NetHost::Impl {
    NetHostConfig Config;
    ENetHost* Host = nullptr;

    std::thread PollThread;
    std::atomic<bool> Running {false};

    // Written only from the poll thread (RegisterPeer/UnregisterPeer run inside PollLoop); read
    // from whichever thread calls Send()/Disconnect() - guarded since those two can race.
    std::mutex PeersMutex;
    std::unordered_map<PeerId, ENetPeer*> PeersById;
    PeerId NextPeerId = 1;

    ConnectCallback OnConnect;
    DisconnectCallback OnDisconnect;
    MessageCallback OnMessage;

    PeerId RegisterPeer(ENetPeer* peer) {
        std::lock_guard<std::mutex> lock(PeersMutex);
        PeerId id = NextPeerId++;
        PeersById[id] = peer;
        peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
        return id;
    }

    void UnregisterPeer(ENetPeer* peer) {
        std::lock_guard<std::mutex> lock(PeersMutex);
        PeersById.erase(static_cast<PeerId>(reinterpret_cast<uintptr_t>(peer->data)));
    }

    ENetPeer* FindPeer(PeerId id) {
        std::lock_guard<std::mutex> lock(PeersMutex);
        auto it = PeersById.find(id);
        return it == PeersById.end() ? nullptr : it->second;
    }

    void PollLoop() {
        ENetEvent event;
        while (Running.load(std::memory_order_relaxed)) {
            // Small timeout keeps this responsive to Shutdown() without busy-spinning.
            while (enet_host_service(Host, &event, 4) > 0) {
                switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    PeerId id = RegisterPeer(event.peer);
                    if (OnConnect)
                        OnConnect(NetPeer {id});
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    PeerId id = static_cast<PeerId>(reinterpret_cast<uintptr_t>(event.peer->data));
                    UnregisterPeer(event.peer);
                    if (OnDisconnect)
                        OnDisconnect(NetPeer {id}, event.data);
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    PeerId id = static_cast<PeerId>(reinterpret_cast<uintptr_t>(event.peer->data));
                    if (OnMessage)
                        OnMessage(NetPeer {id}, event.packet->data, event.packet->dataLength, event.channelID);
                    enet_packet_destroy(event.packet);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
};

NetHost::NetHost() : m_Impl(std::make_unique<Impl>()) {}

NetHost::~NetHost() {
    Shutdown();
}

bool NetHost::Init(const NetHostConfig& config) {
    if (enet_initialize() != 0) {
        WK_CORE_ERROR("NetHost::Init: enet_initialize failed");
        return false;
    }

    m_Impl->Config = config;

    ENetAddress address {};
    ENetAddress* bindAddress = nullptr;
    if (config.Mode == NetHostMode::Server) {
        address.host = ENET_HOST_ANY;
        address.port = config.Port;
        bindAddress = &address;
    }

    size_t maxPeers = config.Mode == NetHostMode::Server ? config.MaxPeers : 1;
    m_Impl->Host = enet_host_create(bindAddress, maxPeers, NetChannelCount, 0, 0);
    if (!m_Impl->Host) {
        WK_CORE_ERROR("NetHost::Init: enet_host_create failed");
        enet_deinitialize();
        return false;
    }

    m_Impl->Running = true;
    m_Impl->PollThread = std::thread([this] { m_Impl->PollLoop(); });
    return true;
}

void NetHost::Shutdown() {
    if (!m_Impl->Host)
        return;

    m_Impl->Running = false;
    if (m_Impl->PollThread.joinable())
        m_Impl->PollThread.join();

    enet_host_destroy(m_Impl->Host);
    m_Impl->Host = nullptr;
    enet_deinitialize();
}

void NetHost::Connect(const std::string& host, uint16_t port) {
    if (!m_Impl->Host) {
        WK_CORE_ERROR("NetHost::Connect: host not initialized");
        return;
    }

    ENetAddress address {};
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    ENetPeer* peer = enet_host_connect(m_Impl->Host, &address, NetChannelCount, 0);
    if (!peer)
        WK_CORE_ERROR("NetHost::Connect: enet_host_connect failed ({0}:{1})", host, port);
    // Success is asynchronous - PollLoop's ENET_EVENT_TYPE_CONNECT case assigns the PeerId and
    // fires OnConnect once ENet finishes the handshake.
}

void NetHost::Send(NetPeer peer, const NetMessage& msg, NetChannel channel, bool reliable) {
    ENetPeer* enetPeer = m_Impl->FindPeer(peer.Id);
    if (!enetPeer) {
        WK_CORE_WARNING("NetHost::Send: unknown or disconnected peer id {0}", peer.Id);
        return;
    }

    NetBuffer wire = EncodeEnvelope(msg);
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(wire.data(), wire.size(), flags);
    enet_peer_send(enetPeer, static_cast<enet_uint8>(channel), packet);
}

void NetHost::Broadcast(const NetMessage& msg, NetChannel channel, bool reliable) {
    if (!m_Impl->Host)
        return;

    NetBuffer wire = EncodeEnvelope(msg);
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(wire.data(), wire.size(), flags);
    enet_host_broadcast(m_Impl->Host, static_cast<enet_uint8>(channel), packet);
}

void NetHost::Disconnect(NetPeer peer, uint32_t reasonCode) {
    ENetPeer* enetPeer = m_Impl->FindPeer(peer.Id);
    if (!enetPeer)
        return;
    enet_peer_disconnect(enetPeer, reasonCode);
}

void NetHost::SetOnConnect(ConnectCallback cb) {
    m_Impl->OnConnect = std::move(cb);
}

void NetHost::SetOnDisconnect(DisconnectCallback cb) {
    m_Impl->OnDisconnect = std::move(cb);
}

void NetHost::SetOnMessage(MessageCallback cb) {
    m_Impl->OnMessage = std::move(cb);
}

} // namespace Wankel::Networking
