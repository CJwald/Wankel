#pragma once

namespace Wankel {

class Scene;

// Writes TransformAnimation's offset to Transform::AnimationPosition/AnimationRotation - runs right before
// TransformSystem::Update, which composes it into LocalTransform so the whole subtree follows that frame.
class TransformAnimationSystem {
public:
    void Update(Scene& scene, float dt);

private:
    float m_TeleportThreshold = 50.0f; // same jump cutoff as KinematicsSystem - a teleport isn't motion
};

} // namespace Wankel
