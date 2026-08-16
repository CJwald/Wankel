#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>


namespace Wankel {

struct Pose {
    glm::vec3 Position {0.0f};
    glm::quat Orientation {1, 0, 0, 0};
};

struct PoseSet {
    std::vector<Pose> Poses;
    int Current = 0; // index into Poses - the active/target pose (next pass: what a PoseSystem tweens toward)
};

} // namespace Wankel
