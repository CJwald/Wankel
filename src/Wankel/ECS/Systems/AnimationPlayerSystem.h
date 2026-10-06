#pragma once

namespace Wankel {

class Scene;

// Advances every AnimationPlayer and writes each target's evaluated pose into its PoseSet for PoseSystem.
class AnimationPlayerSystem {
public:
    void Update(Scene& scene, float dt);
};

} // namespace Wankel
