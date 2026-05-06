#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Wankel/Renderer/Mesh.h"
#include "Entity.h"

namespace Wankel {

    struct TransformComponent {
        glm::vec3 Position{0.0f};
		glm::quat Orientation{1,0,0,0};
        glm::vec3 Scale{1.0f};

		glm::mat4 GetTransform() const {
            return glm::translate(glm::mat4(1.0f), Position)
                 * glm::toMat4(Orientation)
                 * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

	struct MeshComponent {
        Mesh* MeshPtr = nullptr;
    };

    struct CameraComponent {
        float FOV = 66.0f; // Vertical FOV ~= 100 Horizontal on 16:9
        float Near = 0.1f;
        float Far = 1000.0f;
    };

	struct FollowCameraComponent {
        Entity Target;
        glm::vec3 Offset{0.0f, 2.0f, 5.0f};
        glm::quat RotationOffset{1, 0, 0, 0};
    };

	struct PlayerControllerComponent {
        float MoveSpeed = 5.0f;
	    float BoostMultiplier = 1.2f;
	    float BoostDurration = 10.0f;
	    float BoostCooldown = 5.0f;
	    bool Boost = false;
	    
		float WindowSensitivity = 0.002f; 
	    float MouseSensitivity = 2.5f; 
	    float RollSpeed = 2.5f; 
	    
		float LookDeltaX = 0.0f;
		float LookDeltaY = 0.0f;
		glm::vec3 MoveInput{0.0f};
	    float RollInput = 0.0f; 
	    
		glm::quat Orientation{1,0,0,0};
    };

	struct RigidbodyComponent {
	    glm::vec3 Velocity{0.0f};
	    glm::vec3 ForcedVelocity{0.0f};
	    float Mass = 1.0f;
	    bool IsStatic = false;
	};
	
	struct ColliderComponent {
	    enum class Type {
	        AABB,
	        Sphere,
	        Capsule
	    } Type;
	
	    // union or variant later
	    glm::vec3 Size;   // AABB
	    float Radius;     // Sphere
	};

	struct AABBComponent {
    	glm::vec3 HalfSize = {0.5f, 0.5f, 0.5f}; // matches cube mesh
	};

}
