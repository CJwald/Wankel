#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Wankel {

	struct Transform {
	    // SIMULATION
	    glm::vec3 LocalPosition{0.0f};
	    glm::quat LocalOrientation{1,0,0,0};
	    glm::vec3 LocalScale{1.0f};
	
	    // VISUAL OFFSET
	    glm::vec3 VisualPosition{0.0f};
	    glm::quat VisualRotation{1,0,0,0};
	
	    // CACHED TRANSFORMS
	    glm::mat4 LocalTransform{1.0f};
	    glm::mat4 WorldTransform{1.0f};
	    glm::mat4 VisualTransform{1.0f};
	    glm::mat4 FinalTransform{1.0f};
	};


	struct Kinematics {
	    glm::vec3 WorldVelocity{0};
	    glm::vec3 WorldAcceleration{0};
	
	    glm::vec3 WorldAngularVelocity{0};
	    glm::vec3 WorldAngularAcceleration{0};
	
	    glm::vec3 PreviousWorldPosition{0};
	    glm::quat PreviousWorldRotation{1,0,0,0};
	};

}
