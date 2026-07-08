#include "wkpch.h"
#include "TransformSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"
#include "Wankel/ECS/Components/MotionProfile.h"
#include "Wankel/Math/SecondOrderDynamics.h"

#include <glm/gtx/quaternion.hpp>


namespace Wankel {

static constexpr int kMaxHierarchyDepth = 64;

static glm::mat4 ComposeTransform(const Transform& tc) {
    return glm::translate(glm::mat4(1.0f), tc.LocalPosition) * glm::toMat4(tc.LocalOrientation) *
           glm::scale(glm::mat4(1.0f), tc.LocalScale);
}


static glm::mat4 ComputeWorldTransform(entt::registry& registry, entt::entity e, int depth = 0) {
    auto& tc = registry.get<Transform>(e);
    glm::mat4 local = ComposeTransform(tc);

    if (depth >= kMaxHierarchyDepth) {
        WK_CORE_ERROR("Transform hierarchy cycle or excessive depth detected at entity {0} - breaking recursion",
                      (uint32_t)e);
        return local;
    }

    if (registry.all_of<Parent>(e)) {
        auto parent = registry.get<Parent>(e).Parent.GetHandle();
        if (parent != entt::null && parent != e) {
            glm::mat4 parentWorld = ComputeWorldTransform(registry, parent, depth + 1);
            return parentWorld * local;
        }
    }

    return local;
}


void TransformSystem::Update(Scene& scene) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform>();

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);

        tc.LocalTransform = ComposeTransform(tc);
        tc.WorldTransform = ComputeWorldTransform(registry, entity);
    }
}


void TransformSystem::UpdateFinalTransforms(Scene& scene) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform>();

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);
        glm::mat4 visual = glm::translate(glm::mat4(1.0f), tc.VisualPosition) * glm::toMat4(tc.VisualRotation);

        // APPLY VISUAL IN LOCAL SPACE
        tc.FinalTransform = tc.WorldTransform * visual;
    }
}

} // namespace Wankel
