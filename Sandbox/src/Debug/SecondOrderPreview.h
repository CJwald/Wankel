#pragma once

#include <vector>

namespace Wankel {

	class SecondOrderPreview {
	public:
		static std::vector<float> GetStepResponse(
		    float frequency,
		    float damping,
		    float response,
		    int samples = 100,
		    float dt = 1.0f / 100.0f
		);
	};

}
