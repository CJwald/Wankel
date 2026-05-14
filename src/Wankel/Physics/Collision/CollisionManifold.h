#pragma once
#include <glm/glm.hpp>

namespace Wankel {

	struct CollisionManifold {
	    bool Colliding = false;
	    glm::vec3 Normal = {0,0,0};
	    float Penetration = 0.0f;
	};

}
