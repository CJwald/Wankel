#include "wkpch.h"
#include "Scene.h"
#include "Components.h"

#include <chrono>


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

} // namespace

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

    auto t1 = std::chrono::high_resolution_clock::now();
    m_PoseSystem.Update(*this, dt);
    Smooth(m_SystemTimings.PoseMs, ElapsedMs(t1));

    auto t2 = std::chrono::high_resolution_clock::now();
    m_PhysicsSystem.Update(*this, dt);
    Smooth(m_SystemTimings.PhysicsMs, ElapsedMs(t2));

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
