#include "wkpch.h"
#include "RotationPivotSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

#include <glm/gtx/quaternion.hpp>


namespace Wankel {

void RotationPivotSystem::Update(Scene& scene) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, RotationPivot>();

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);
        auto& pivot = view.get<RotationPivot>(entity);

        // Same defensive shape as PlayerControllerSystem's OrientationTarget redirect - a pivot with a
        // missing/invalid/controller-less Source just stays wherever it last was.
        if (!registry.valid(pivot.Source) || !registry.all_of<PlayerController>(pivot.Source))
            continue;

        auto& controller = registry.get<PlayerController>(pivot.Source);

        uint8_t channels = OrientationChannel_None;
        switch (controller.Mode) {
            case PlayerController::LookMode::FPS: channels = pivot.FPSChannels; break;
            case PlayerController::LookMode::Flight: channels = pivot.FlightChannels; break;
            case PlayerController::LookMode::Spectator: channels = pivot.SpectatorChannels; break;
        }

        if (channels == OrientationChannel_None) {
            tc.LocalOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        } else if (channels == OrientationChannel_All) {
            // The only correct representation of Flight's true 6DOF state - not decomposable into
            // independent yaw/pitch/roll scalars without reintroducing gimbal lock.
            tc.LocalOrientation = controller.Orientation;
        } else {
            glm::quat q(1.0f, 0.0f, 0.0f, 0.0f);
            if (channels & OrientationChannel_Yaw)
                q = controller.BodyOrientation * q; // pure yaw in FPS/Spectator
            if (channels & OrientationChannel_Pitch)
                // Local right axis - correct because the parent pivot already applied yaw via
                // hierarchy composition, so no need to rotate about a world-space right vector.
                q = glm::angleAxis(controller.Pitch, glm::vec3(1, 0, 0)) * q;
            if (channels & OrientationChannel_Roll)
                q = glm::angleAxis(controller.Roll, glm::vec3(0, 0, 1)) * q;
            tc.LocalOrientation = glm::normalize(q);
        }
    }
}

} // namespace Wankel
