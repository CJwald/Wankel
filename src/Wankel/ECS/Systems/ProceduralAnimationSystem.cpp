#include "wkpch.h"
#include "ProceduralAnimationSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"
#include "Wankel/ECS/Components/MotionProfile.h"
#include "Wankel/Math/SecondOrderDynamics.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

static void InitProceduralMotion(ProceduralMotion& motion) {
    if (motion.Initialized)
        return;

    for (int in = 0; in < ProceduralMotion::AxisCount; in++) {
        for (int out = 0; out < ProceduralMotion::AxisCount; out++) {
            auto& link = motion.Links[in][out];
            link.Spring = SecondOrderDynamics(link.Frequency, link.Damping, link.Response, 0.0f);
        }
    }

    motion.Initialized = true;
}

void UpdateProceduralMotion(ProceduralMotion& motion, const float (&input)[6], float dt) {
    InitProceduralMotion(motion);

    float output[(int)MotionAxis::Count] = {0};
    for (int in = 0; in < (int)MotionAxis::Count; in++) {
        for (int out = 0; out < (int)MotionAxis::Count; out++) {
            auto& link = motion.Links[in][out];

            if (!link.Enabled)
                continue;

            float target = input[in] * link.Magnitude;
            target = glm::clamp(target, link.ClampMin, link.ClampMax);

            link.Spring.SetDynamics(link.Frequency, link.Damping, link.Response);
            link.Output = link.Spring.Update(dt, target);
            output[out] += link.Output;
        }
    }

    motion.PositionOffset =
        glm::vec3(output[(int)MotionAxis::X], output[(int)MotionAxis::Y], output[(int)MotionAxis::Z]);
    motion.RotationOffset =
        glm::vec3(output[(int)MotionAxis::Pitch], output[(int)MotionAxis::Yaw], output[(int)MotionAxis::Roll]);
}


void ProceduralAnimationSystem::Update(Scene& scene, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, Kinematics, MeshAnimation>();

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);
        auto& kc = view.get<Kinematics>(entity);
        auto& anim = view.get<MeshAnimation>(entity);

        glm::quat worldRot = glm::quat_cast(tc.WorldTransform);
        glm::quat invRot = glm::inverse(worldRot);

        glm::vec3 localVel = invRot * kc.WorldVelocity;
        glm::vec3 localAngVel = invRot * kc.WorldAngularVelocity;

        float input[(int)MotionAxis::Count] = {localVel.x,    localVel.y,    localVel.z,
                                               localAngVel.x, localAngVel.y, localAngVel.z};
        UpdateProceduralMotion(anim, input, dt);

        glm::vec3 rotRad = glm::radians(anim.RotationOffset);
        tc.VisualRotation = glm::normalize(glm::quat(rotRad));

        // Rotate around anim.RotationOrigin instead of the local origin: T(pivot)*R*T(-pivot) collapses to
        // T(pivot - R*pivot)*R, so folding that into the existing Translate*Rotate composition
        // (TransformSystem::UpdateFinalTransforms) just needs this extra offset added to the position term.
        glm::vec3 pivot = anim.RotationOrigin;
        tc.VisualPosition = anim.PositionOffset + pivot - (tc.VisualRotation * pivot);
    }
}

} // namespace Wankel
