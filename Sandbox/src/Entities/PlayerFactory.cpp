#include "PlayerFactory.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Assets/AssetManager.h>
#include <Wankel/Renderer/Mesh.h>

#include <glm/gtc/quaternion.hpp>

using namespace Wankel;

Entity PlayerFactory::Create(Scene& scene) {
    Ref<Mesh> headMesh = AssetManager::GetMesh("Assets/Mesh/PlayerHead01.glb");
    Ref<Mesh> legMesh = AssetManager::GetMesh("Assets/Mesh/PlayerLeg01.glb");
    Ref<Mesh> legMeshMirrored = AssetManager::GetMirroredMesh("Assets/Mesh/PlayerLeg01.glb", true, false, false);
    Ref<Mesh> gunMesh = AssetManager::GetMesh("Assets/Mesh/AK74_IRONS.ply");

    const Material bodyMaterial {{0.75f, 0.76f, 0.78f}, 0.4f, 0.6f, {0.0f, 0.0f, 0.0f}};

    // PLAYER ROOT
    Entity player = scene.CreateEntity();
    player.AddComponent<Tag>().Name = "Player";
    auto& pt = player.AddComponent<Transform>();
    player.AddComponent<Kinematics>();
    pt.LocalPosition = {0.0f, 1.0f, 0.0f};
    player.AddComponent<PlayerController>();
    auto& collider = player.AddComponent<SphereCollider>();
    collider.Radius = 0.5f;
    auto& rb = player.AddComponent<Rigidbody>();
    rb.Velocity = {0.0f, 0.0f, 0.0f};
    rb.IsStatic = false;
    player.AddComponent<Movement>();

    // HEAD
    {
        Entity head = scene.CreateChild(player, "Player Head");
        head.AddComponent<MeshRenderer>().MeshPtr = headMesh.get();
        head.AddComponent<Material>(bodyMaterial);
        auto& anim = head.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::X, 0.5f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -0.5f, 1.6f, 1.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Pitch, 0.2f, 1.6f, 1.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Yaw, 5.0f, 0.6f, 1.6f, 1.6f, 45.0f);
        anim.SetLink(MotionAxis::Pitch, MotionAxis::Pitch, 5.0f, 0.6f, 1.6f, 1.6f, 45.0f);
    }

    // LEG FR
    {
        Entity leg = scene.CreateChild(player, "Player Leg FR");
        leg.GetComponent<Transform>().LocalPosition = {0.6f, 0.0f, -0.6f};
        leg.AddComponent<MeshRenderer>().MeshPtr = legMesh.get();
        leg.AddComponent<Material>(bodyMaterial);
        auto& anim = leg.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Y, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Pitch, 5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Roll, -1.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Pitch, -1.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Roll, 0.5f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Pitch, -0.5f, 1.6f, 0.45f, 0.25f, 45.0f);
    }

    // LEG BR
    {
        Entity leg = scene.CreateChild(player, "Player Leg BR");
        auto& tc = leg.GetComponent<Transform>();
        tc.LocalPosition = {0.6f, 0.0f, 0.6f};
        tc.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                              glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
        leg.AddComponent<MeshRenderer>().MeshPtr = legMesh.get();
        leg.AddComponent<Material>(bodyMaterial);
        auto& anim = leg.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Y, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -5.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Pitch, 5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Roll, -1.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Pitch, -1.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Roll, 2.5f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Pitch, -2.5f, 1.6f, 0.45f, 0.25f, 45.0f);
    }

    // LEG FL (mirrored mesh)
    {
        Entity leg = scene.CreateChild(player, "Player Leg FL");
        leg.GetComponent<Transform>().LocalPosition = {-0.6f, 0.0f, -0.6f};
        leg.AddComponent<MeshRenderer>().MeshPtr = legMeshMirrored.get();
        leg.AddComponent<Material>(bodyMaterial);
        auto& anim = leg.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Y, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Pitch, 5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Roll, -1.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Pitch, -1.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Roll, 2.5f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Pitch, -2.5f, 1.6f, 0.45f, 0.25f, 45.0f);
    }

    // LEG BL (mirrored mesh)
    {
        Entity leg = scene.CreateChild(player, "Player Leg BL");
        auto& tc = leg.GetComponent<Transform>();
        tc.LocalPosition = {-0.6f, 0.0f, 0.6f};
        tc.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                              glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
        leg.AddComponent<MeshRenderer>().MeshPtr = legMeshMirrored.get();
        leg.AddComponent<Material>(bodyMaterial);
        auto& anim = leg.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Y, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::X, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Z, -1.0f * 0.01f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Pitch, 5.0f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Roll, -1.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Y, MotionAxis::Pitch, -1.0f, 1.6f, 0.6f, 1.4f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Roll, 2.5f, 1.6f, 0.45f, 0.25f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Pitch, -2.5f, 1.6f, 0.45f, 0.25f, 45.0f);
    }

    // GUN
    {
        Entity gun = scene.CreateChild(player, "Gun1");
        gun.GetComponent<Transform>().LocalPosition = {0.06f, -0.06f, -0.8f};
        gun.AddComponent<MeshRenderer>().MeshPtr = gunMesh.get();
        gun.AddComponent<Material>(Material {{0.4f, 0.4f, 0.4f}, 0.75f, 0.0f, {0.0f, 0.0f, 0.0f}});
        auto& anim = gun.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -1.2f, 1.8f, 0.8f, 2.0f, 45.0f);
        anim.SetLink(MotionAxis::Z, MotionAxis::Z, 0.002f, 1.8f, 0.8f, 2.0f, 45.0f);
        anim.SetLink(MotionAxis::Yaw, MotionAxis::Yaw, 5.0f, 1.8f, 1.3f, 0.5f, 45.0f);
        anim.SetLink(MotionAxis::Pitch, MotionAxis::Pitch, 3.5f, 1.8f, 1.3f, 0.5f, 45.0f);
    }

    // CAMERA
    {
        Entity camEntity = scene.CreateChild(player, "Player Camera");
        camEntity.AddComponent<CameraComponent>();
        auto& tc = camEntity.GetComponent<Transform>();
        tc.LocalPosition = {0.0f, 0.3f, 3.35f};
        tc.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 1, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
        auto& anim = camEntity.AddComponent<MeshAnimation>();
        anim.SetLink(MotionAxis::X, MotionAxis::Roll, -1.2f, 1.8f, 0.8f, 2.0f, 45.0f);
    }

    return player;
}
