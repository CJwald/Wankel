#pragma once

namespace Wankel {

class Scene;

class KinematicsSystem {
public:
    void Update(Scene& scene, float dt);

private:
    float m_dPosThreshold = 50.0f;
};
} // namespace Wankel
