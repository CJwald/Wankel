#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/ECS/Components/MotionProfile.h"


namespace Wankel {

	struct MeshAnimation {
		static constexpr int AxisCount = (int)MotionAxis::Count;

    	MotionLink Links[AxisCount][AxisCount];

    	glm::vec3 PositionOffset{0.0f};
    	glm::vec3 RotationOffset{0.0f};

    	bool Initialized = false;
	};

}
