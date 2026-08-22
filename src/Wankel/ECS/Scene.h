#pragma once
#include "Entity.h"
#include "Wankel/ECS/Components/HiearchyComponents.h"
#include "Wankel/ECS/Serialization/ComponentRegistry.h"
#include "Wankel/Physics/Systems/PhysicsSystem.h"
#include "Wankel/ECS/Systems/PlayerControllerSystem.h"
#include "Wankel/ECS/Systems/PoseSystem.h"
#include "Wankel/ECS/Systems/TransformSystem.h"
#include "Wankel/ECS/Systems/KinematicsSystem.h"
#include "Wankel/ECS/Systems/ProceduralAnimationSystem.h"
#include "Wankel/ECS/Systems/CameraSystem.h"
#include "Wankel/Renderer/Camera.h"

#include <nlohmann/json.hpp>

namespace Wankel {

// Per-system CPU cost of the last Scene::OnUpdate call, smoothed (exponential moving average) so a
// debug display is readable rather than jumping every frame. Milliseconds, one field per system-
// stage in the exact order OnUpdate runs them - see its own comment for why TransformSystem appears
// twice (once before animation, once after, folding the animation's visual offset into
// FinalTransform).
struct SceneSystemTimings {
    float PlayerControllerMs = 0.0f;
    float PoseMs = 0.0f;
    float PhysicsMs = 0.0f;
    float TransformMs = 0.0f;
    float KinematicsMs = 0.0f;
    float ProceduralAnimationMs = 0.0f;
    float TransformFinalMs = 0.0f;
    float CameraMs = 0.0f;
};

class Scene {
public:
    Entity CreateEntity() { return Entity(m_Registry.create(), &m_Registry); }

    // Creates an entity with the Tag/Transform/Kinematics/Parent quintet that's
    // identical across every "spawn a child part" call site (head/legs/gun/camera
    // parented to a player, body/legs parented to an enemy, etc) - the caller adds
    // whatever else the specific part needs (MeshRenderer/Material/MeshAnimation/...).
    Entity CreateChild(Entity parent, const std::string& name);

    // Recurses into children (a mob's body/leg meshes, a player's head/legs/gun/camera, ...) rather
    // than detaching and orphaning them - destroying a hierarchy root should remove the whole rig.
    void DestroyEntity(Entity entity) {
        auto handle = entity.GetHandle();

        auto view = m_Registry.view<Parent>();
        std::vector<entt::entity> children;
        for (auto e : view) {
            if (view.get<Parent>(e).Parent.GetHandle() == handle)
                children.push_back(e);
        }
        for (auto child : children)
            DestroyEntity(Entity(child, &m_Registry));

        m_Registry.destroy(handle);
    }

    void OnUpdate(float dt, Camera& camera);

    entt::registry& Registry() { return m_Registry; }

    // Tuning-only JSON snapshot of entity's engine-owned components (Transform, colliders,
    // MeshAnimation, PlayerController, Rigidbody, Movement, MeshRenderer's transform/mirror/layer
    // fields) - one object keyed by component name, missing components simply absent from the
    // output. See Serialization::GetComponentTable() (ECS/Serialization/ComponentRegistry.h) for the
    // exact set and ComponentSerialization.h for which fields of each are considered tuning vs.
    // runtime state. Mechtrix-owned gameplay components (Health/Faction/MobController/Weapon) are not
    // covered here - the game layer composes its own equivalent alongside this.
    nlohmann::json SerializeEntity(Entity entity);

    // Applies a JSON object produced by SerializeEntity back onto entity, adding any component present
    // in json that entity doesn't already have. Unknown keys are ignored (forward-compatible); keys
    // absent from json leave the corresponding component untouched.
    void DeserializeEntity(Entity entity, const nlohmann::json& json);

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

    // Per-system CPU timing from the most recent OnUpdate, for a debug/profiling overlay - see
    // SceneSystemTimings' own comment.
    const SceneSystemTimings& GetSystemTimings() const { return m_SystemTimings; }

private:
    PlayerControllerSystem m_PlayerControllerSystem;
    PoseSystem m_PoseSystem;
    TransformSystem m_TransformSystem;
    KinematicsSystem m_KinematicsSystem;
    ProceduralAnimationSystem m_ProceduralAnimationSystem;
    CameraSystem m_CameraSystem;

    entt::registry m_Registry;
    PhysicsSystem m_PhysicsSystem;
    SceneSystemTimings m_SystemTimings;
};

} // namespace Wankel
