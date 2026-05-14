#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Wankel/Renderer/Mesh.h"
#include "Entity.h"
#include "Wankel/Math/SecondOrderDynamics.h"

namespace Wankel {

    struct TransformComponent {
        glm::vec3 LocalPosition{0.0f};
		glm::quat LocalOrientation{1,0,0,0};
        glm::vec3 LocalScale{1.0f};

		glm::mat4 WorldTransform{1.0f};

		glm::mat4 GetLocalTransform() const {
            return glm::translate(glm::mat4(1.0f), LocalPosition)
                 * glm::toMat4(LocalOrientation)
                 * glm::scale(glm::mat4(1.0f), LocalScale);
        }

		glm::vec3 GetWorldPosition() const {
            return glm::vec3(WorldTransform[3]);
        }

        glm::quat GetWorldRotation() const {
            return glm::quat_cast(WorldTransform);
        }
    };


	struct MeshComponent {
        Mesh* MeshPtr = nullptr;

		glm::vec3 LocalPosition{0.0f};
    	glm::quat LocalRotation{1,0,0,0};
    	glm::vec3 LocalScale{1.0f};

    	glm::vec3 RotationPivot{0.0f};
		glm::mat4 GetLocalTransform() const {

            glm::mat4 pivotToOrigin = glm::translate(glm::mat4(1.0f), -RotationPivot);

            glm::mat4 pivotBack = glm::translate(glm::mat4(1.0f), RotationPivot);

            return glm::translate(glm::mat4(1.0f), LocalPosition) * 
				pivotBack * glm::toMat4(LocalRotation) * 
				pivotToOrigin * glm::scale(glm::mat4(1.0f), LocalScale);
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


	struct PlayerControllerComponent {
		enum class LookMode {
			FPS,
			Flight
		};

        float MoveSpeed = 5.0f;
	    float BoostMultiplier = 1.2f;
	    bool Boost = false;
	    
		float WindowSensitivity = 0.002f; 
	    float MouseSensitivity = 2.5f; 
	    float RollSpeed = 2.5f; 
	    
		float LookDeltaX = 0.0f;
		float LookDeltaY = 0.0f;
		glm::vec3 MoveInput{0.0f};
	    float RollInput = 0.0f; 
	    
		LookMode Mode = LookMode::FPS;

		// FPS CAMERA STATE
		float Yaw = 0.0f;
		float Pitch = 0.0f;
		float Roll = 0.0f;
		float MaxPitchUp = glm::radians(89.0f);
		float MaxPitchDown = glm::radians(-89.0f);

		glm::quat Orientation{1,0,0,0};
		glm::quat BodyOrientation{1,0,0,0}; // Needed for FPS mode
    };


	struct ParentComponent {
	    Entity Parent;
	};


	struct ChildrenComponent {
	    std::vector<Entity> Children;
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


	struct MeshAnimationComponent {
	
	    // FINAL VISUAL OFFSETS
	    glm::vec3 PositionOffset{0.0f};
	    glm::vec3 RotationOffset{0.0f};
	
	    // TARGETS
	    glm::vec3 TargetPosition{0.0f};
	    glm::vec3 TargetRotation{0.0f};
	
	    // SPRINGS
	    SecondOrderDynamics PositionSpring;
	    SecondOrderDynamics RotationSpring;
	
	    // TUNING

		glm::vec3 PositionAmplitude{0.001f, 0.001f, 0.001f};
    	glm::vec3 RotationAmplitude{0.005f, 0.005f, 0.005f};

	    float PositionFrequency = 2.0f;
	    float PositionDamping = 0.8f;
	    float PositionResponse = 2.0f;
	
	    float RotationFrequency = 2.0f;
	    float RotationDamping = 0.8f;
	    float RotationResponse = 2.0f;
	
	    bool Initialized = false;
	};

}
