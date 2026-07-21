#pragma once

#include <glm/glm.hpp>

namespace Wankel {
class Scene;
}

// Draws debug wireframes for every entity in the scene: per-entity axes +
// parent-link lines, AABB collider boxes, and Sphere collider circles.
// No-ops unless Renderer::DebugEnabled is set. Called once per visible
// chunk tile from the tiled-world render loop, hence the worldOffset param.
class ColliderDebugDraw {
public:
    static void Draw(Wankel::Scene& scene, const glm::vec3& worldOffset);
};
