#pragma once

#include <cstdint>

namespace Wankel::Networking {

// Engine-owned message ids: 0-999. Game-layer ids (Mechtrix's WorldDelta broadcasts, weapon RPCs,
// etc.) start at GameMessageRangeStart (1000) - Wankel never defines anything at or above that, so
// the two id spaces can never collide without either side needing to know about the other's enum.
enum class NetMessageType : uint16_t {
    Handshake = 1,
    Disconnect = 2,
    Ping = 3,
    Pong = 4,

    EntitySnapshot = 10,
    EntitySpawn = 11,
    EntityDestroy = 12,

    GameMessageRangeStart = 1000,
};

} // namespace Wankel::Networking
