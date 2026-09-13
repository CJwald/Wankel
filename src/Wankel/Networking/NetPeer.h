#pragma once

#include "NetCommon.h"

namespace Wankel::Networking {

// Opaque handle to a connected peer - the underlying ENet peer pointer stays private to
// NetHost.cpp, so nothing outside Networking/ ever needs to know ENet exists.
struct NetPeer {
    PeerId Id = InvalidPeerId;

    bool IsValid() const { return Id != InvalidPeerId; }
    bool operator==(const NetPeer& other) const { return Id == other.Id; }
};

} // namespace Wankel::Networking
