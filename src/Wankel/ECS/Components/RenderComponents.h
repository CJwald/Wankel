#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Wankel/Renderer/Mesh.h"
#include "Wankel/ECS/Entity.h"


namespace Wankel {

	struct MeshRenderer {
        Mesh* MeshPtr = nullptr;

		glm::vec3 LocalPosition{0.0f};
    	glm::quat LocalRotation{1,0,0,0};
    	glm::vec3 LocalScale{1.0f};

		bool MirrorX = false;
		bool MirrorY = false;
		bool MirrorZ = false;

    	glm::vec3 RotationPivot{0.0f};

		glm::mat4 GetLocalTransform() const {
            glm::mat4 pivotToOrigin = glm::translate(glm::mat4(1.0f), -RotationPivot);
            glm::mat4 pivotBack = glm::translate(glm::mat4(1.0f), RotationPivot);
			glm::vec3 finalScale = LocalScale;
			if (MirrorX) finalScale.x *= -1.0f;
			if (MirrorY) finalScale.y *= -1.0f;
			if (MirrorZ) finalScale.z *= -1.0f;
            return glm::translate(glm::mat4(1.0f), LocalPosition) * 
				pivotBack * glm::toMat4(LocalRotation) * 
				pivotToOrigin * glm::scale(glm::mat4(1.0f), finalScale);
        }
    };


    struct CameraComponent {
        float FOV = 66.0f; // Vertical FOV ~= 100 Horizontal on 16:9
        float Near = 0.1f;
        float Far = 1000.0f;
    };


	struct FollowCameraComponent {
        Entity Target;
        glm::vec3 Offset{0.0f, 0.0f, 0.0f};
        glm::quat RotationOffset{1, 0, 0, 0};
    };

}
