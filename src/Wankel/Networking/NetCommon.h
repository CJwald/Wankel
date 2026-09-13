#pragma once

#include <cstdint>

namespace Wankel::Networking {

// Stable id for a connected peer, assigned by NetHost on connect - stays valid for the lifetime of
// that connection. 0 is reserved as "no peer" (see NetPeer::IsValid).
using PeerId = uint32_t;
constexpr PeerId InvalidPeerId = 0;

} // namespace Wankel::Networking
