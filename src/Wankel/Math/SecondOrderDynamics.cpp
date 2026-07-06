#include "wkpch.h"
#include "SecondOrderDynamics.h"

#include <algorithm>
#include <cmath>

#include "Math.h"

namespace Wankel {

	SecondOrderDynamics::SecondOrderDynamics(float frequency, float damping, float response, float initialValue) {
		SetDynamics(frequency, damping, response);

	    m_PreviousInput = initialValue;
	    m_Output = initialValue;
	    m_OutputVelocity = 0.0f;
	}


	float SecondOrderDynamics::Update(float dt, float target) {
	    if (dt <= 0.0f)
	        return m_Output;

	    float inputVelocity = (target - m_PreviousInput) / dt;

	    m_PreviousInput = target;
	
	    float stableK2 = std::max( 
			m_K2, 
			1.1f * (dt * dt / 4.0f + dt * m_K1 / 2.0f)
		);
	
	    // Integrate position
	    m_Output += m_OutputVelocity * dt;
	
	    // Acceleration
	    float acceleration = (target + m_K3 * inputVelocity - m_Output - m_K1 * m_OutputVelocity) / stableK2;
	
	    // Integrate velocity
	    m_OutputVelocity += acceleration * dt;
	
	    return m_Output;
	}
	
	
	void SecondOrderDynamics::Reset(float value) {
	    m_PreviousInput = value;
	    m_Output = value;
	    m_OutputVelocity = 0.0f;
	}
	
	
	void SecondOrderDynamics::SetDynamics(float frequency, float damping, float response) {
	    frequency = std::max(frequency, 0.001f);

	    m_K1 = damping / (Math::PI * frequency);
	    m_K2 = 1.0f / ((2.0f * Math::PI * frequency) * (2.0f * Math::PI * frequency));
	    m_K3 = response * damping / (2.0f * Math::PI * frequency);
	}

}
