#pragma once

#include "NetMessage.h"
#include "NetMessageTypes.h"
#include "NetPeer.h"

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace Wankel::Networking {

// Raw bytes arrive off the main thread (from NetHost's poll thread, via its OnMessage callback).
// NetMessageBus decodes just the envelope there (pure CPU work, safe off-thread), then marshals
// the decoded message onto the main thread via JobSystem::SubmitMainThread before invoking the
// registered handler for its type - so handlers can safely touch Scene/the ECS registry.
//
// Handlers are a plain type->callback table (mirrors ReplicationRegistry's function-pointer-table
// idiom) rather than a switch statement, so registering a new message type never means editing this
// class.
class NetMessageBus {
public:
    using Handler = std::function<void(NetPeer, const NetMessage&)>;

    void RegisterHandler(NetMessageType type, Handler handler);

    // Call this from NetHost::SetOnMessage. Safe to call from any thread.
    void Dispatch(NetPeer peer, const uint8_t* data, size_t len);

private:
    std::unordered_map<NetMessageType, Handler> m_Handlers;
};

} // namespace Wankel::Networking
