#pragma once

#include <nlohmann/json.hpp>

// Hand-written, tuning-only JSON views of the engine's components - deliberately not named
// to_json/from_json (which nlohmann::json would find via ADL and call implicitly from generic code,
// e.g. `json j = someComponent;`), so a caller always opts in explicitly and never accidentally gets
// a full struct dump that includes per-frame runtime state (velocities, cooldowns, spring state, ...).
// See Scene::SerializeEntity/DeserializeEntity (Scene.h) for the entity-level driver, and
// ComponentRegistry.h for how these get dispatched by component type.

namespace Wankel {

struct Transform;
struct SphereCollider;
struct AABBCollider;
struct CapsuleCollider;
struct MeshAnimation;
struct PlayerController;
struct Rigidbody;
struct Movement;
struct MeshRenderer;

namespace Serialization {

// LocalPosition/LocalOrientation/LocalScale only - VisualPosition/VisualRotation and the cached
// Local/World/FinalTransform matrices are runtime-derived, not authored.
nlohmann::json Serialize(const Transform& transform);
void Deserialize(const nlohmann::json& json, Transform& transform);

nlohmann::json Serialize(const SphereCollider& collider);
void Deserialize(const nlohmann::json& json, SphereCollider& collider);

nlohmann::json Serialize(const AABBCollider& collider);
void Deserialize(const nlohmann::json& json, AABBCollider& collider);

nlohmann::json Serialize(const CapsuleCollider& collider);
void Deserialize(const nlohmann::json& json, CapsuleCollider& collider);

// Only enabled Links[][] slots are written, each as {From, To, <MotionLink::CopyTuning fields>}, plus
// PositionOffset/RotationOffset/RotationOrigin. Spring/Output (per-link runtime state) and Initialized
// are skipped.
nlohmann::json Serialize(const MeshAnimation& animation);
void Deserialize(const nlohmann::json& json, MeshAnimation& animation);

// Tuning fields only - skips Boost/LookDeltaX/LookDeltaY/RollInput/MoveInput/Yaw/Pitch/Roll/
// Orientation/BodyOrientation/R3PressedLastFrame (all per-frame input/camera state).
nlohmann::json Serialize(const PlayerController& controller);
void Deserialize(const nlohmann::json& json, PlayerController& controller);

// Mass/IsStatic/GravityScale only - skips Velocity/Force.
nlohmann::json Serialize(const Rigidbody& rigidbody);
void Deserialize(const nlohmann::json& json, Rigidbody& rigidbody);

// MaxSpeed/Acceleration/Deceleration only - skips MoveIntent and the SavedMaxSpeed hack field.
nlohmann::json Serialize(const Movement& movement);
void Deserialize(const nlohmann::json& json, Movement& movement);

// Transform/mirror/layer fields only - deliberately no MeshPtr entry. A live Mesh* can't be turned
// back into an asset path (Mesh carries no path, AssetManager has no reverse lookup), so mesh identity
// stays archetype data (e.g. RigPart::MeshPath in Mechtrix) rather than something this serializer
// round-trips.
nlohmann::json Serialize(const MeshRenderer& renderer);
void Deserialize(const nlohmann::json& json, MeshRenderer& renderer);

} // namespace Serialization
} // namespace Wankel
