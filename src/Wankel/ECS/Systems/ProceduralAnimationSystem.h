#pragma once

namespace Wankel {

class Scene;
struct ProceduralMotion;

// Steps every enabled link's spring toward input[in] * Magnitude and writes the summed per-axis output
// to PositionOffset/RotationOffset. input = local {vel.x, vel.y, vel.z, angVel.x, angVel.y, angVel.z}.
void UpdateProceduralMotion(ProceduralMotion& motion, const float (&input)[6], float dt);

class ProceduralAnimationSystem {
public:
    void Update(Scene& scene, float dt);
};

} // namespace Wankel
