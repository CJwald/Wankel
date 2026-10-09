#include "wkpch.h"
#include "Scene.h"
#include "Components.h"

#include <chrono>
#include <cmath>


namespace Wankel {

namespace {

float ElapsedMs(const std::chrono::high_resolution_clock::time_point& start) {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(end - start).count();
}

// Exponential moving average - stable enough to actually read on a debug overlay, responsive enough
// to still reflect a real regression within a few dozen frames rather than washing it out entirely.
constexpr float kTimingSmoothing = 0.1f;

void Smooth(float& average, float sample) {
    average += (sample - average) * kTimingSmoothing;
}

// A removed TransformAnimation would otherwise leave its last offset baked into the entity's pose.
void ClearAnimationOffset(entt::registry& registry, entt::entity entity) {
    if (auto* tc = registry.try_get<Transform>(entity)) {
        tc->AnimationPosition = glm::vec3(0.0f);
        tc->AnimationRotation = glm::quat(1, 0, 0, 0);
    }
}

} // namespace

Scene::Scene() {
    m_Registry.on_destroy<TransformAnimation>().connect<&ClearAnimationOffset>();
}

Entity Scene::CreateChild(Entity parent, const std::string& name) {
    Entity child = CreateEntity();
    child.AddComponent<Tag>().Name = name;
    child.AddComponent<Transform>();
    child.AddComponent<Kinematics>();
    child.AddComponent<Parent>().Parent = parent;
    return child;
}

nlohmann::json Scene::SerializeEntity(Entity entity) {
    nlohmann::json json;
    for (auto& entry : Serialization::GetComponentTable()) {
        nlohmann::json componentJson;
        if (entry.Serialize(m_Registry, entity.GetHandle(), componentJson))
            json[entry.Key] = std::move(componentJson);
    }
    return json;
}

void Scene::DeserializeEntity(Entity entity, const nlohmann::json& json) {
    for (auto& entry : Serialization::GetComponentTable()) {
        auto it = json.find(entry.Key);
        if (it != json.end())
            entry.Deserialize(m_Registry, entity.GetHandle(), *it);
    }
}

void Scene::OnUpdate(float dt, Camera& camera) {
    auto t0 = std::chrono::high_resolution_clock::now();
    m_PlayerControllerSystem.Update(*this, dt);
    Smooth(m_SystemTimings.PlayerControllerMs, ElapsedMs(t0));

    auto tRotationPivot = std::chrono::high_resolution_clock::now();
    m_RotationPivotSystem.Update(*this);
    Smooth(m_SystemTimings.RotationPivotMs, ElapsedMs(tRotationPivot));

    // Velocity -> clip -> per-target pose, ahead of PoseSystem, which blends those poses into the local transforms.
    auto tLocomotion = std::chrono::high_resolution_clock::now();
    m_LocomotionSystem.Update(*this);
    Smooth(m_SystemTimings.LocomotionMs, ElapsedMs(tLocomotion));

    auto tAnimationPlayer = std::chrono::high_resolution_clock::now();
    m_AnimationPlayerSystem.Update(*this, dt);
    Smooth(m_SystemTimings.AnimationPlayerMs, ElapsedMs(tAnimationPlayer));

    auto t1 = std::chrono::high_resolution_clock::now();
    m_PoseSystem.Update(*this, dt);
    Smooth(m_SystemTimings.PoseMs, ElapsedMs(t1));

    auto t2 = std::chrono::high_resolution_clock::now();
    // Fixed steps covering this frame's time (see TimestepSettings), then render positions eased by the leftover.
    const TimestepSettings& timestep = m_PhysicsSystem.Timestep;
    float step = glm::max(timestep.FixedDeltaTime, 1e-4f);
    m_PhysicsAccumulator += dt;
    int steps = 0;
    while (m_PhysicsAccumulator >= step && steps < timestep.MaxStepsPerFrame) {
        m_PhysicsSystem.Update(*this, step);
        m_PhysicsAccumulator -= step;
        steps++;
    }
    if (m_PhysicsAccumulator >= step)
        m_PhysicsAccumulator = std::fmod(m_PhysicsAccumulator, step); // over budget - drop the backlog, don't spiral
    m_PhysicsSystem.Interpolate(*this, m_PhysicsAccumulator / step);
    m_SystemTimings.PhysicsSteps = steps;
    Smooth(m_SystemTimings.PhysicsMs, ElapsedMs(t2));

    // Right before the hierarchy pass that consumes its offsets - animated parts and their children
    // resolve this frame's pose together, with no one-frame lag.
    auto tTransformAnimation = std::chrono::high_resolution_clock::now();
    m_TransformAnimationSystem.Update(*this, dt);
    Smooth(m_SystemTimings.TransformAnimationMs, ElapsedMs(tTransformAnimation));

    auto t3 = std::chrono::high_resolution_clock::now();
    m_TransformSystem.Update(*this);
    Smooth(m_SystemTimings.TransformMs, ElapsedMs(t3));

    auto t4 = std::chrono::high_resolution_clock::now();
    m_KinematicsSystem.Update(*this, dt);
    Smooth(m_SystemTimings.KinematicsMs, ElapsedMs(t4));

    auto t5 = std::chrono::high_resolution_clock::now();
    m_ProceduralAnimationSystem.Update(*this, dt);
    Smooth(m_SystemTimings.ProceduralAnimationMs, ElapsedMs(t5));

    auto t6 = std::chrono::high_resolution_clock::now();
    m_TransformSystem.UpdateFinalTransforms(*this);
    Smooth(m_SystemTimings.TransformFinalMs, ElapsedMs(t6));

    auto t7 = std::chrono::high_resolution_clock::now();
    m_CameraSystem.Update(*this, camera);
    Smooth(m_SystemTimings.CameraMs, ElapsedMs(t7));
}
} // namespace Wankel
