#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Wankel {

	struct Rigidbody {
	    glm::vec3 Velocity{0.0f};
	    glm::vec3 Force{0.0f}; // Not used, I think I want it eventually

	    float Mass = 1.0f;
	    bool IsStatic = false;
	};
	

	struct Collider {
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


	struct AABBCollider {
    	glm::vec3 HalfSize = {0.5f, 0.5f, 0.5f};
		glm::vec3 Offset{0.0f};
	};


	struct SphereCollider {
	    float Radius = 0.5f;
		glm::vec3 Offset{0.0f};
	};

}
