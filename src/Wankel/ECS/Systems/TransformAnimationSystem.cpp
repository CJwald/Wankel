#include "wkpch.h"
#include "TransformAnimationSystem.h"

#include "ProceduralAnimationSystem.h"
#include "Wankel/ECS/Components.h"
#include "Wankel/ECS/Scene.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

namespace {

// Rotation of a possibly-scaled matrix - quat_cast needs orthonormal columns.
glm::quat RotationOf(const glm::mat4& m) {
    glm::mat3 r(glm::normalize(glm::vec3(m[0])), glm::normalize(glm::vec3(m[1])), glm::normalize(glm::vec3(m[2])));
    return glm::normalize(glm::quat_cast(r));
}

} // namespace

void TransformAnimationSystem::Update(Scene& scene, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, TransformAnimation>();
    float safeDt = glm::max(dt, 1e-6f);

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);
        auto& anim = view.get<TransformAnimation>(entity);

        // Input is the rest frame's motion (parent world x unanimated local), not Kinematics - the entity's own
        // world motion includes this offset and would feed back into its springs. Parent world is last frame's.
        glm::mat4 rest = glm::translate(glm::mat4(1.0f), tc.LocalPosition) * glm::toMat4(tc.LocalOrientation);
        if (auto* parent = registry.try_get<Parent>(entity)) {
            entt::entity parentHandle = parent->Parent.GetHandle();
            if (parentHandle != entt::null && parentHandle != entity && registry.all_of<Transform>(parentHandle))
                rest = registry.get<Transform>(parentHandle).WorldTransform * rest;
        }
        glm::vec3 restPosition = glm::vec3(rest[3]);
        glm::quat restOrientation = RotationOf(rest);

        glm::vec3 worldVel(0.0f);
        glm::vec3 worldAngVel(0.0f);
        if (anim.HasPreviousRest && glm::length(restPosition - anim.PreviousRestPosition) < m_TeleportThreshold) {
            worldVel = (restPosition - anim.PreviousRestPosition) / safeDt;

            glm::quat delta = restOrientation * glm::inverse(anim.PreviousRestOrientation);
            float angle = glm::angle(delta);
            if (angle > glm::pi<float>())
                angle -= glm::two_pi<float>();
            if (glm::abs(angle) > 0.00001f)
                worldAngVel = glm::axis(delta) * (angle / safeDt);
        }
        anim.PreviousRestPosition = restPosition;
        anim.PreviousRestOrientation = restOrientation;
        anim.HasPreviousRest = true;

        glm::quat invRest = glm::inverse(restOrientation);
        glm::vec3 localVel = invRest * worldVel;
        glm::vec3 localAngVel = invRest * worldAngVel;
        float input[(int)MotionAxis::Count] = {localVel.x,    localVel.y,    localVel.z,
                                               localAngVel.x, localAngVel.y, localAngVel.z};
        UpdateProceduralMotion(anim, input, dt);

        // Same pivot folding as ProceduralAnimationSystem: T(pivot)*R*T(-pivot) == T(pivot - R*pivot)*R.
        tc.AnimationRotation = glm::normalize(glm::quat(glm::radians(anim.RotationOffset)));
        glm::vec3 pivot = anim.RotationOrigin;
        tc.AnimationPosition = anim.PositionOffset + pivot - (tc.AnimationRotation * pivot);
    }
}

} // namespace Wankel
