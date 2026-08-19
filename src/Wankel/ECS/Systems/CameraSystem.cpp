#include "wkpch.h"
#include "CameraSystem.h"

#include "Wankel/Renderer/Camera.h"
#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"
#include "Wankel/ECS/Components/MotionProfile.h"

//#include <glm/gtx/quaternion.hpp>


namespace Wankel {

void CameraSystem::Update(Scene& scene, Camera& camera) {
    auto view = scene.Registry().view<Transform, CameraComponent>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& cam = view.get<CameraComponent>(entity);
        if (!cam.Primary)
            continue;

        // POSITION - FinalTransform (not WorldTransform) so any MeshAnimation-driven VisualPosition
        // offset on this entity (e.g. procedural sway) reaches the actual render camera.
        glm::vec3 position = glm::vec3(transform.FinalTransform[3]);
        camera.SetPosition(position);

        // ROTATION
        glm::quat rotation = glm::quat_cast(transform.FinalTransform);
        camera.SetOrientation(rotation);

        camera.SetFOV(cam.FOV);
        camera.SetNearClip(cam.Near);
        camera.SetFarClip(cam.Far);
        camera.SetCullingMask(cam.CullingMask);

        break;
    }
}
} // namespace Wankel
