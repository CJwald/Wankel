#include "wkpch.h"
#include "Scene.h"
#include "Components.h"


namespace Wankel {

Entity Scene::CreateChild(Entity parent, const std::string& name) {
    Entity child = CreateEntity();
    child.AddComponent<Tag>().Name = name;
    child.AddComponent<Transform>();
    child.AddComponent<Kinematics>();
    child.AddComponent<Parent>().Parent = parent;
    return child;
}

void Scene::OnUpdate(float dt, Camera& camera) {
    m_PlayerControllerSystem.Update(*this, dt);
    m_PhysicsSystem.Update(*this, dt);
    m_TransformSystem.Update(*this);
    m_KinematicsSystem.Update(*this, dt);
    m_ProceduralAnimationSystem.Update(*this, dt);
    m_TransformSystem.UpdateFinalTransforms(*this);
    m_CameraSystem.Update(*this, camera);
}
} // namespace Wankel
