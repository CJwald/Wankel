#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Wankel {

	struct PlayerController {
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
		bool R3PressedLastFrame = false;

		// FPS CAMERA STATE, TODO: make sure I need these
		float Yaw = 0.0f;
		float Pitch = 0.0f;
		float Roll = 0.0f;
		float MaxPitchUp = glm::radians(89.0f);
		float MaxPitchDown = glm::radians(-89.0f);

		glm::quat Orientation{1,0,0,0};
		glm::quat BodyOrientation{1,0,0,0}; // Needed for FPS mode
    };


	struct Movement {
	    glm::vec3 MoveIntent{0.0f};

	    float MaxSpeed = 5.0f; 
		float SavedMaxSpeed = 5.0f; // TODO: Remove, this is a hack to get boost working before full refactor
		float Acceleration = 50.0f;
		float Deceleration = 50.0f; // low decel rate with fast acc fells good for space flight
	};


}
