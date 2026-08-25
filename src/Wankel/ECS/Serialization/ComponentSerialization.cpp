#include "wkpch.h"

#include "ComponentSerialization.h"

#include "Wankel/ECS/Components/AnimationComponents.h"
#include "Wankel/ECS/Components/InputComponents.h"
#include "Wankel/ECS/Components/MotionProfile.h"
#include "Wankel/ECS/Components/PhysicsComponents.h"
#include "Wankel/ECS/Components/RenderComponents.h"
#include "Wankel/ECS/Components/TransformComponents.h"

using nlohmann::json;

namespace Wankel {

namespace {

json ToJson(const glm::vec3& v) {
    return json {v.x, v.y, v.z};
}

void FromJson(const json& j, glm::vec3& v) {
    v = {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>()};
}

// {w, x, y, z} - matches glm::quat's own constructor argument order.
json ToJson(const glm::quat& q) {
    return json {q.w, q.x, q.y, q.z};
}

void FromJson(const json& j, glm::quat& q) {
    q = {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>(), j.at(3).get<float>()};
}

MotionAxis MotionAxisFromName(const std::string& name) {
    for (int i = 0; i < (int)MotionAxis::Count; i++) {
        auto axis = (MotionAxis)i;
        if (name == MotionAxisName(axis))
            return axis;
    }
    return MotionAxis::Count; // caller must treat this as "not found"
}

} // namespace

namespace Serialization {

json Serialize(const Transform& transform) {
    return {
        {"LocalPosition", ToJson(transform.LocalPosition)},
        {"LocalOrientation", ToJson(transform.LocalOrientation)},
        {"LocalScale", ToJson(transform.LocalScale)},
    };
}

void Deserialize(const json& json, Transform& transform) {
    FromJson(json.at("LocalPosition"), transform.LocalPosition);
    FromJson(json.at("LocalOrientation"), transform.LocalOrientation);
    FromJson(json.at("LocalScale"), transform.LocalScale);
}

json Serialize(const SphereCollider& collider) {
    return {
        {"Radius", collider.Radius},
        {"Offset", ToJson(collider.Offset)},
        {"Friction", collider.Friction},
    };
}

void Deserialize(const json& json, SphereCollider& collider) {
    collider.Radius = json.at("Radius").get<float>();
    FromJson(json.at("Offset"), collider.Offset);
    collider.Friction = json.at("Friction").get<float>();
}

json Serialize(const AABBCollider& collider) {
    return {
        {"HalfSize", ToJson(collider.HalfSize)},
        {"Offset", ToJson(collider.Offset)},
        {"Friction", collider.Friction},
    };
}

void Deserialize(const json& json, AABBCollider& collider) {
    FromJson(json.at("HalfSize"), collider.HalfSize);
    FromJson(json.at("Offset"), collider.Offset);
    collider.Friction = json.at("Friction").get<float>();
}

json Serialize(const CapsuleCollider& collider) {
    return {
        {"Radius", collider.Radius},
        {"HalfHeight", collider.HalfHeight},
        {"Offset", ToJson(collider.Offset)},
        {"Friction", collider.Friction},
    };
}

void Deserialize(const json& json, CapsuleCollider& collider) {
    collider.Radius = json.at("Radius").get<float>();
    collider.HalfHeight = json.at("HalfHeight").get<float>();
    FromJson(json.at("Offset"), collider.Offset);
    collider.Friction = json.at("Friction").get<float>();
}

json Serialize(const MeshAnimation& animation) {
    json links = json::array();
    for (int from = 0; from < MeshAnimation::AxisCount; from++) {
        for (int to = 0; to < MeshAnimation::AxisCount; to++) {
            const MotionLink& link = animation.Links[from][to];
            if (!link.Enabled)
                continue;

            links.push_back({
                {"From", MotionAxisName((MotionAxis)from)},
                {"To", MotionAxisName((MotionAxis)to)},
                {"Magnitude", link.Magnitude},
                {"Frequency", link.Frequency},
                {"Damping", link.Damping},
                {"Response", link.Response},
                {"ClampMin", link.ClampMin},
                {"ClampMax", link.ClampMax},
            });
        }
    }

    return {
        {"Links", links},
        {"PositionOffset", ToJson(animation.PositionOffset)},
        {"RotationOffset", ToJson(animation.RotationOffset)},
        {"RotationOrigin", ToJson(animation.RotationOrigin)},
    };
}

void Deserialize(const json& json, MeshAnimation& animation) {
    animation = {}; // clear any previously-enabled links before applying the saved set

    for (const auto& linkJson : json.at("Links")) {
        MotionAxis from = MotionAxisFromName(linkJson.at("From").get<std::string>());
        MotionAxis to = MotionAxisFromName(linkJson.at("To").get<std::string>());
        if (from == MotionAxis::Count || to == MotionAxis::Count) {
            WK_CORE_WARNING("MeshAnimation::Deserialize: unknown axis name in saved link, skipping");
            continue;
        }

        MotionLink& link = animation.Links[(int)from][(int)to];
        link.Enabled = true;
        link.Magnitude = linkJson.at("Magnitude").get<float>();
        link.Frequency = linkJson.at("Frequency").get<float>();
        link.Damping = linkJson.at("Damping").get<float>();
        link.Response = linkJson.at("Response").get<float>();
        link.ClampMin = linkJson.at("ClampMin").get<float>();
        link.ClampMax = linkJson.at("ClampMax").get<float>();
    }

    FromJson(json.at("PositionOffset"), animation.PositionOffset);
    FromJson(json.at("RotationOffset"), animation.RotationOffset);
    FromJson(json.at("RotationOrigin"), animation.RotationOrigin);
}

json Serialize(const PlayerController& controller) {
    return {
        {"MoveSpeed", controller.MoveSpeed},
        {"BoostMultiplier", controller.BoostMultiplier},
        {"WindowSensitivity", controller.WindowSensitivity},
        {"MouseSensitivity", controller.MouseSensitivity},
        {"RollSpeed", controller.RollSpeed},
        {"ControllerSensitivity", controller.ControllerSensitivity},
        {"LookCurve", (int)controller.LookCurve},
        {"LookCurveExponent", controller.LookCurveExponent},
        {"FPSDeceleration", controller.FPSDeceleration},
        {"FlightDeceleration", controller.FlightDeceleration},
        {"FlightGravityScale", controller.FlightGravityScale},
        {"MaxPitchUp", controller.MaxPitchUp},
        {"MaxPitchDown", controller.MaxPitchDown},
        {"Mode", (int)controller.Mode},
    };
}

void Deserialize(const json& json, PlayerController& controller) {
    controller.MoveSpeed = json.at("MoveSpeed").get<float>();
    controller.BoostMultiplier = json.at("BoostMultiplier").get<float>();
    controller.WindowSensitivity = json.at("WindowSensitivity").get<float>();
    controller.MouseSensitivity = json.at("MouseSensitivity").get<float>();
    controller.RollSpeed = json.at("RollSpeed").get<float>();

    // Absent in archetype JSON saved before these were added - leave PlayerController's own defaults
    // (1.0/Linear/1.5) in place rather than throwing (json.at would abort the whole Deserialize call).
    if (json.contains("ControllerSensitivity"))
        controller.ControllerSensitivity = glm::max(json.at("ControllerSensitivity").get<float>(), 0.05f);
    if (json.contains("LookCurve"))
        controller.LookCurve = (EaseType)json.at("LookCurve").get<int>();
    if (json.contains("LookCurveExponent"))
        controller.LookCurveExponent = json.at("LookCurveExponent").get<float>();

    controller.FPSDeceleration = json.at("FPSDeceleration").get<float>();
    controller.FlightDeceleration = json.at("FlightDeceleration").get<float>();
    controller.FlightGravityScale = json.at("FlightGravityScale").get<float>();
    controller.MaxPitchUp = json.at("MaxPitchUp").get<float>();
    controller.MaxPitchDown = json.at("MaxPitchDown").get<float>();
    controller.Mode = (PlayerController::LookMode)json.at("Mode").get<int>();
}

json Serialize(const Rigidbody& rigidbody) {
    return {
        {"Mass", rigidbody.Mass},
        {"IsStatic", rigidbody.IsStatic},
        {"GravityScale", rigidbody.GravityScale},
    };
}

void Deserialize(const json& json, Rigidbody& rigidbody) {
    rigidbody.Mass = json.at("Mass").get<float>();
    rigidbody.IsStatic = json.at("IsStatic").get<bool>();
    rigidbody.GravityScale = json.at("GravityScale").get<float>();
}

json Serialize(const Movement& movement) {
    return {
        {"MaxSpeed", movement.MaxSpeed},
        {"Acceleration", movement.Acceleration},
        {"Deceleration", movement.Deceleration},
    };
}

void Deserialize(const json& json, Movement& movement) {
    movement.MaxSpeed = json.at("MaxSpeed").get<float>();
    movement.Acceleration = json.at("Acceleration").get<float>();
    movement.Deceleration = json.at("Deceleration").get<float>();
    movement.SavedMaxSpeed = movement.MaxSpeed;
}

json Serialize(const MeshRenderer& renderer) {
    return {
        {"LocalPosition", ToJson(renderer.LocalPosition)},
        {"LocalRotation", ToJson(renderer.LocalRotation)},
        {"LocalScale", ToJson(renderer.LocalScale)},
        {"MirrorX", renderer.MirrorX},
        {"MirrorY", renderer.MirrorY},
        {"MirrorZ", renderer.MirrorZ},
        {"RotationPivot", ToJson(renderer.RotationPivot)},
        {"Layers", renderer.Layers},
    };
}

void Deserialize(const json& json, MeshRenderer& renderer) {
    FromJson(json.at("LocalPosition"), renderer.LocalPosition);
    FromJson(json.at("LocalRotation"), renderer.LocalRotation);
    FromJson(json.at("LocalScale"), renderer.LocalScale);
    renderer.MirrorX = json.at("MirrorX").get<bool>();
    renderer.MirrorY = json.at("MirrorY").get<bool>();
    renderer.MirrorZ = json.at("MirrorZ").get<bool>();
    FromJson(json.at("RotationPivot"), renderer.RotationPivot);
    renderer.Layers = json.at("Layers").get<uint32_t>();
}

} // namespace Serialization
} // namespace Wankel
