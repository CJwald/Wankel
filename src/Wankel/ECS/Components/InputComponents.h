#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Wankel {

struct PlayerController {
    // Spectator: always-upright free-fly - yaw about world up (never the tilted local up Flight uses,
    // so it can never bank/roll), movement follows the full pitched look direction (unlike FPS, which
    // flattens movement to the body plane). See PlayerControllerSystem::Update.
    enum class LookMode { FPS, Flight, Spectator };

    float MoveSpeed = 5.0f;
    float BoostMultiplier = 1.2f;
    bool Boost = false;

    float WindowSensitivity = 0.002f;
    float MouseSensitivity = 2.5f;
    float RollSpeed = 2.5f;

    float LookDeltaX = 0.0f;
    float LookDeltaY = 0.0f;
    float RollInput = 0.0f;
    glm::vec3 MoveInput {0.0f};

    LookMode Mode = LookMode::FPS;
    bool R3PressedLastFrame = false;

    // Per-mode Movement::Deceleration, applied each frame by PlayerControllerSystem (mirrors how
    // Boost/MaxSpeed already works below) - Mech/FPS stays snappy (matches Movement's own prior
    // single shared default); any non-grounded mode (Flight, Spectator) uses FlightDeceleration
    // instead - Flight's default drifts slowly to a stop, Spectator typically wants this cranked way
    // up for an instant halt. Tune to taste - time-to-stop from MaxSpeed is roughly MaxSpeed/this.
    float FPSDeceleration = 50.0f;
    float FlightDeceleration = 2.0f;

    // Per-mode Rigidbody::GravityScale, applied each frame by PlayerControllerSystem (same pattern as
    // FPSDeceleration/FlightDeceleration above) - Mech/FPS is always fully grounded (hardcoded 1.0 in
    // PlayerControllerSystem, not exposed as its own tunable); any non-grounded mode (Flight,
    // Spectator) uses FlightGravityScale, defaulting to 0 (free-flight, unaffected by world gravity
    // unless you deliberately dial this up toward 1).
    float FlightGravityScale = 0.0f;

    // FPS CAMERA STATE, TODO: make sure I need these
    float Yaw = 0.0f;
    float Pitch = 0.0f;
    float Roll = 0.0f;
    float MaxPitchUp = glm::radians(89.0f);
    float MaxPitchDown = glm::radians(-89.0f);

    glm::quat Orientation {1, 0, 0, 0};
    glm::quat BodyOrientation {1, 0, 0, 0}; // Needed for FPS mode
};


struct Movement {
    glm::vec3 MoveIntent {0.0f};

    float MaxSpeed = 5.0f;
    float SavedMaxSpeed = 5.0f; // TODO: Remove, this is a hack to get boost working before full refactor
    float Acceleration = 50.0f;
    // Default/standalone value for a Movement not driven by a PlayerController (e.g. future
    // Movement-driven AI) - an entity with both gets this overwritten every frame from
    // PlayerController::FPSDeceleration/FlightDeceleration instead (see PlayerControllerSystem).
    float Deceleration = 50.0f;
};


} // namespace Wankel
