#include "wkpch.h"
#include "SecondOrderPreview.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


namespace Wankel {

	std::vector<float> SecondOrderPreview::GetStepResponse(
	    float frequency,
	    float damping,
	    float response,
	    int samples,
	    float dt
	)
	{
		std::vector<float> values(samples);

		float y  = 0.0f;
		float yd = 0.0f;

		float w = glm::max(frequency, 0.001f);
		float z = damping;
		float r = response;

		float pi = glm::pi<float>();

		float k1 = z / (pi * w);

		float k2 =
		    1.0f /
		    ((2.0f * pi * w) *
		     (2.0f * pi * w));

		float k3 = r * z / (2.0f * pi * w);

		float target = 1.0f;
		float xd = 1.0f;

		for (int i = 0; i < samples; i++) {
			float x = target;

			y += dt * yd;

			float accel = (x + k3 * xd - y - k1 * yd) / k2;

			yd += dt * accel;

			values[i] = y;
		}

		return values;
	}

}
