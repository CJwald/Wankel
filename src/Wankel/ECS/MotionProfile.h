#pragma once

#include "Wankel/Math/SecondOrderDynamics.h"

namespace Wankel {

	enum class MotionAxis : uint8_t {
	    PosX = 0,
	    PosY,
	    PosZ,
	    RotX,
	    RotY,
	    RotZ,
	
	    Count
	};
	
	struct MotionLink {
	    bool Enabled = false;
	
	    float Magnitude = 0.0f;
	
	    float Frequency = 2.0f;
	    float Damping = 0.8f;
	    float Response = 2.0f;
	
	    float Clamp = 9999.0f;
	
	    SecondOrderDynamics Spring;
	};

}
