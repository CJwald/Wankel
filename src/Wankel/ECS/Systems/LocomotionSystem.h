#pragma once

namespace Wankel {

class Scene;

// Picks Idle/Moving per Locomotion from its Rigidbody's speed and plays the matching clip on its AnimationPlayer.
class LocomotionSystem {
public:
    void Update(Scene& scene);
};

} // namespace Wankel
