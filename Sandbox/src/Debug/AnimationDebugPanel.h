#pragma once

#include <entt/entt.hpp>

namespace Wankel {
class Scene;
}

// ImGui panel: an entity picker + a MeshAnimation link editor (add/remove/
// tweak links, with a SecondOrderPreview step-response plot per link).
// selectedEntity persists the dropdown selection across frames/calls.
class AnimationDebugPanel {
public:
    static void Draw(Wankel::Scene& scene, entt::entity& selectedEntity);
};
