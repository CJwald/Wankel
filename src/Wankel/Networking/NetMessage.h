#pragma once

#include "NetMessageTypes.h"

#include <cstdint>
#include <vector>

namespace Wankel::Networking {

using NetBuffer = std::vector<uint8_t>;

// Wire envelope for every message: a type tag, a reserved tick/sequence number, and an
// already-encoded, type-specific body. Tick is unused until client-side prediction/reconciliation
// is designed in a later phase - reserving it now avoids a breaking wire-format change to every
// message type once that lands.
//
// `serialize()` below is a template, so it has no bitsery dependency here - only the translation
// unit that actually instantiates it (NetSerialize.h's EncodeEnvelope/DecodeEnvelope) needs to
// include bitsery, keeping this header lightweight for anything that just passes NetMessage around.
struct NetMessage {
    NetMessageType Type = NetMessageType::Handshake;
    uint32_t Tick = 0;
    NetBuffer Payload;
};

// Max encoded body size accepted off the wire - guards DecodeEnvelope against a corrupt/hostile
// length prefix trying to make the receiver allocate something absurd.
constexpr size_t MaxNetMessagePayloadBytes = 64 * 1024;

template <typename S>
void serialize(S& s, NetMessage& m) {
    s.value2b(m.Type);
    s.value4b(m.Tick);
    s.container1b(m.Payload, MaxNetMessagePayloadBytes);
}

} // namespace Wankel::Networking
