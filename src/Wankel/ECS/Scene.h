#pragma once
#include "Entity.h"
#include "Wankel/ECS/Components/HiearchyComponents.h"
#include "Wankel/Physics/Systems/PhysicsSystem.h"
#include "Wankel/ECS/Systems/PlayerControllerSystem.h"
#include "Wankel/ECS/Systems/TransformSystem.h"
#include "Wankel/ECS/Systems/KinematicsSystem.h"
#include "Wankel/ECS/Systems/ProceduralAnimationSystem.h"
#include "Wankel/ECS/Systems/CameraSystem.h"
#include "Wankel/Renderer/Camera.h"

namespace Wankel {

class Scene {
public:
    Entity CreateEntity() { return Entity(m_Registry.create(), &m_Registry); }

    // Creates an entity with the Tag/Transform/Kinematics/Parent quintet that's
    // identical across every "spawn a child part" call site (head/legs/gun/camera
    // parented to a player, body/legs parented to an enemy, etc) - the caller adds
    // whatever else the specific part needs (MeshRenderer/Material/MeshAnimation/...).
    Entity CreateChild(Entity parent, const std::string& name);

    void DestroyEntity(Entity entity) {
        auto handle = entity.GetHandle();

        auto view = m_Registry.view<Parent>();
        for (auto e : view) {
            if (view.get<Parent>(e).Parent.GetHandle() == handle)
                m_Registry.remove<Parent>(e);
        }

        m_Registry.destroy(handle);
    }

    void OnUpdate(float dt, Camera& camera);

    entt::registry& Registry() { return m_Registry; }

    // Forwards to PhysicsSystem - call after adding/removing/moving a static collider entity
    // (e.g. terrain regeneration) so the cached static broad-phase grid picks it up.
    void MarkStaticCollidersDirty() { m_PhysicsSystem.MarkStaticCollidersDirty(); }

    // Forwards to PhysicsSystem - prefer these over MarkStaticCollidersDirty() when only a few
    // static colliders changed (e.g. voxel terrain edits), since a full rebuild costs O(whole
    // static collider set) regardless of how many entities actually changed.
    void UpdateStaticCollider(Entity entity, const AABB& worldBounds) {
        m_PhysicsSystem.UpdateStaticCollider(entity.GetHandle(), worldBounds);
    }
    void RemoveStaticCollider(Entity entity) { m_PhysicsSystem.RemoveStaticCollider(entity.GetHandle()); }

    // Forwards to PhysicsSystem - see GravitySettings for what's tunable (Enabled/Magnitude/Direction).
    GravitySettings& GetGravitySettings() { return m_PhysicsSystem.Gravity; }

private:
    PlayerControllerSystem m_PlayerControllerSystem;
    TransformSystem m_TransformSystem;
    KinematicsSystem m_KinematicsSystem;
    ProceduralAnimationSystem m_ProceduralAnimationSystem;
    CameraSystem m_CameraSystem;

    entt::registry m_Registry;
    PhysicsSystem m_PhysicsSystem;
};

} // namespace Wankel
