#include "wkpch.h"

#include "NetDispatch.h"

#include "NetSerialize.h"
#include "Wankel/Core/JobSystem.h"

namespace Wankel::Networking {

void NetMessageBus::RegisterHandler(NetMessageType type, Handler handler) {
    m_Handlers[type] = std::move(handler);
}

void NetMessageBus::Dispatch(NetPeer peer, const uint8_t* data, size_t len) {
    auto decoded = DecodeEnvelope(data, len);
    if (!decoded) {
        WK_CORE_WARNING("NetMessageBus: failed to decode envelope from peer {0}", peer.Id);
        return;
    }

    auto it = m_Handlers.find(decoded->Type);
    if (it == m_Handlers.end())
        return; // no handler registered for this message type - not an error, just unhandled

    Handler handler = it->second;
    JobSystem::SubmitMainThread([handler, peer, message = std::move(*decoded)]() mutable {
        handler(peer, message);
    });
}

} // namespace Wankel::Networking
