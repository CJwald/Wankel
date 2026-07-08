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

        // POSITION
        glm::vec3 position = glm::vec3(transform.WorldTransform[3]);
        camera.SetPosition(position);

        // ROTATION
        glm::quat rotation = glm::quat_cast(transform.WorldTransform);
        camera.SetOrientation(rotation);

        camera.SetFOV(cam.FOV);
        camera.SetNearClip(cam.Near);
        camera.SetFarClip(cam.Far);

        break;
    }
}
} // namespace Wankel
