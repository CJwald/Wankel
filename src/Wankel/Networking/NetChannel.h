#pragma once

#include <cstdint>

namespace Wankel::Networking {

// Reliable: handshake, world/session state, voxel edit diffs, one-shot RPC events (weapon fired,
// spawn/destroy, chat) - must arrive, order matters.
// UnreliableState: per-tick transform/velocity/aim snapshots - ENet sequences packets per-channel
// and drops anything older than the latest received automatically, so a dropped or late sample is
// simply superseded rather than delaying newer data.
enum class NetChannel : uint8_t { Reliable = 0, UnreliableState = 1 };

constexpr uint8_t NetChannelCount = 2;

} // namespace Wankel::Networking
