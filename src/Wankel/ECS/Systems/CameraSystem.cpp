#include "wkpch.h"
#include "CameraSystem.h"

#include "Wankel/Renderer/Camera.h"
#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

//#include <glm/gtx/quaternion.hpp>


namespace Wankel {
	
	void CameraSystem::Update(Scene& scene, Camera& camera) {
		// FOLLOW CAMERA SYSTEM
		auto& registry = scene.Registry();
		auto camView = registry.view<Transform, FollowCameraComponent>();
		
		for (auto entity : camView) {
		    //auto& transform = camView.get<Transform>(entity);
		    auto& follow = camView.get<FollowCameraComponent>(entity);
		
		    if (!follow.Target)
		        continue;
		
		    auto& targetTransform = follow.Target.GetComponent<Transform>();
		
		    // BUILD TARGET TRANSFORM
			glm::mat4 targetMat = targetTransform.WorldTransform;
		
		    // BUILD CAMERA OFFSET
		    glm::mat4 offsetMat =
		        glm::translate(glm::mat4(1.0f), follow.Offset) *
		        glm::mat4_cast(follow.RotationOffset);
		
		    // FINAL CAMERA TRANSFORM
		    glm::mat4 cameraMat = targetMat * offsetMat;
		
		    // POSITION
		    glm::vec3 cameraPos = glm::vec3(cameraMat[3]);
		    camera.SetPosition(cameraPos);
		
		    // ROTATION
		    glm::mat4 rotMat = cameraMat;
		    rotMat[3] = glm::vec4(0,0,0,1);
		    glm::quat cameraRot = glm::quat_cast(rotMat);
		    camera.SetOrientation(cameraRot);
		}
	}
}
