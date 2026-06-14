#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Wankel/Renderer/Mesh.h"
#include "Entity.h"
#include "Wankel/Math/SecondOrderDynamics.h"
#include "MotionProfile.h"

namespace Wankel {

	struct TransformComponent {
	    // SIMULATION
	    glm::vec3 LocalPosition{0.0f};
	    glm::quat LocalOrientation{1,0,0,0};
	    glm::vec3 LocalScale{1.0f};
	
	    // VISUAL OFFSET
	    glm::vec3 VisualPosition{0.0f};
	    glm::quat VisualRotation{1,0,0,0};
	
	    // KINEMATICS
	    glm::vec3 PreviousWorldPosition{0.0f};
	    glm::quat PreviousWorldRotation{1,0,0,0};
	
	    glm::vec3 WorldVelocity{0.0f};
	    glm::vec3 WorldAngularVelocity{0.0f};
		glm::vec3 WorldAcceleration{0.0f};
		glm::vec3 WorldAngularAcceleration{0.0f};
	
	    // CACHED TRANSFORMS
	    glm::mat4 LocalTransform{1.0f};
	    glm::mat4 WorldTransform{1.0f};
	    glm::mat4 VisualTransform{1.0f};
	    glm::mat4 FinalTransform{1.0f};
	};


	struct MeshComponent {
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


	struct TagComponent {
	    std::string Name;
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
	    float RollInput = 0.0f; 
		glm::vec3 MoveInput{0.0f};
	    
		LookMode Mode = LookMode::FPS;

		// FPS CAMERA STATE, TODO: make sure I need these
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
	
	    bool InheritPosition = true;
	    bool InheritRotation = true;
	    bool InheritScale = true;
	
	    bool InheritLinearVelocity = true;
	    bool InheritAngularVelocity = true;
	};


	struct ChildrenComponent {
	    std::vector<Entity> Children;
	};


	struct MovementComponent {
	    glm::vec3 MoveIntent{0.0f};

	    float MaxSpeed = 5.0f; 
		float SavedMaxSpeed = 5.0f; // TODO: Remove, this is a hack to get boost working before full refactor
		float Acceleration = 50.0f;
		float Deceleration = 50.0f; // low decel rate with fast acc fells good for space flight
	};


	struct RigidbodyComponent {
	    glm::vec3 Velocity{0.0f};
	    glm::vec3 Force{0.0f}; // Not used, I think I want it eventually

	    float Mass = 1.0f;
	    bool IsStatic = false;
	};
	

	struct ColliderComponent {
	    enum class Type : uint8_t {
	        AABB,
	        Sphere,
	        Capsule
	    };

		Type type;
	
	    glm::vec3 HalfSize{0.5f};   // AABB
	    float Radius = 0.5f;        // Sphere
	    float CapsuleHalfHeight = 0.5f; // Capsule

	    glm::vec3 Offset{0.0f};
	};


	struct AABBComponent {
    	glm::vec3 HalfSize = {0.5f, 0.5f, 0.5f};
		glm::vec3 Offset{0.0f};
	};


	struct SphereColliderComponent {
	    float Radius = 0.5f;
		glm::vec3 Offset{0.0f};
	};


	struct MeshAnimationComponent {
		static constexpr int AxisCount = (int)MotionAxis::Count;

    	MotionLink Links[AxisCount][AxisCount];

    	glm::vec3 PositionOffset{0.0f};
    	glm::vec3 RotationOffset{0.0f};

    	bool Initialized = false;
	};

}
