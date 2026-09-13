#pragma once

#include "NetMessage.h"

#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>

#include <optional>

namespace Wankel::Networking {

using OutputAdapter = bitsery::OutputBufferAdapter<NetBuffer>;
using InputAdapter = bitsery::InputBufferAdapter<NetBuffer>;

// Encodes `value` (any type with a discoverable `serialize(S&, T&)` free function) to a
// tightly-sized buffer. quickSerialization's output container can end up with size() larger than
// the actual encoded byte count (its adapter over-allocates rather than shrinking back down), so
// every encode site must trim to the returned writtenBytesCount - otherwise a nested container
// (e.g. NetMessage::Payload) picks up trailing garbage, and quickDeserialization's "fully consumed"
// check fails on the receiving end. Always go through this helper rather than calling
// bitsery::quickSerialization directly.
template <typename T>
NetBuffer EncodeBytes(const T& value) {
    NetBuffer buf;
    size_t written = bitsery::quickSerialization<OutputAdapter>(buf, value);
    buf.resize(written);
    return buf;
}

// Builds a NetMessage envelope by bitsery-encoding `body` (any type with a discoverable
// `serialize(S&, T&)` free function) into the envelope's Payload.
template <typename T>
NetMessage MakeMessage(NetMessageType type, uint32_t tick, const T& body) {
    NetMessage msg;
    msg.Type = type;
    msg.Tick = tick;
    msg.Payload = EncodeBytes(body);
    return msg;
}

// Encodes a full envelope (type + tick + already-encoded payload bytes) to wire-ready bytes.
inline NetBuffer EncodeEnvelope(const NetMessage& msg) {
    return EncodeBytes(msg);
}

// Decodes just the envelope from raw bytes off the wire. Callers dispatch on the returned
// message's Type, then use DecodeBody<T> to interpret its Payload as the concrete body type that
// message type is known to carry.
inline std::optional<NetMessage> DecodeEnvelope(const uint8_t* data, size_t len) {
    NetMessage msg;
    NetBuffer wire(data, data + len);
    auto state = bitsery::quickDeserialization<InputAdapter>({wire.begin(), wire.size()}, msg);
    if (state.first != bitsery::ReaderError::NoError || !state.second)
        return std::nullopt;
    return msg;
}

// Decodes a message body of type T from an already-extracted NetMessage::Payload.
template <typename T>
std::optional<T> DecodeBody(const NetBuffer& payload) {
    T body {};
    auto state = bitsery::quickDeserialization<InputAdapter>({payload.begin(), payload.size()}, body);
    if (state.first != bitsery::ReaderError::NoError || !state.second)
        return std::nullopt;
    return body;
}

} // namespace Wankel::Networking
