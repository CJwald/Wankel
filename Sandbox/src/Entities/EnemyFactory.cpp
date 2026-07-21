#include "EnemyFactory.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Assets/AssetManager.h>
#include <Wankel/Renderer/Mesh.h>

#include <glm/gtc/quaternion.hpp>

using namespace Wankel;

void EnemyFactory::Create(Scene& scene) {
    Ref<Mesh> bodyMesh = AssetManager::GetMesh("Assets/Mesh/StalkerBody01.ply");
    Ref<Mesh> legMesh = AssetManager::GetMesh("Assets/Mesh/StalkerLeg01.ply");
    Ref<Mesh> legMeshMirrored = AssetManager::GetMirroredMesh("Assets/Mesh/StalkerLeg01.ply", true, false, false);

    Entity enemy = scene.CreateEntity();
    enemy.AddComponent<Tag>().Name = "Enemy";
    auto& et = enemy.AddComponent<Transform>();
    enemy.AddComponent<Kinematics>();
    et.LocalPosition = {2.0f, 1.0f, 0.0f};

    {
        Entity body = scene.CreateChild(enemy, "Enemy Body");
        body.AddComponent<MeshRenderer>().MeshPtr = bodyMesh.get();
    }

    {
        Entity leg = scene.CreateChild(enemy, "Enemy Leg FL");
        leg.GetComponent<Transform>().LocalPosition = {0.6f, 0.0f, -0.6f};
        leg.AddComponent<MeshRenderer>().MeshPtr = legMesh.get();
    }

    {
        Entity leg = scene.CreateChild(enemy, "Enemy Leg BL");
        auto& tc = leg.GetComponent<Transform>();
        tc.LocalPosition = {0.6f, 0.0f, 0.6f};
        tc.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                              glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
        leg.AddComponent<MeshRenderer>().MeshPtr = legMesh.get();
    }

    {
        Entity leg = scene.CreateChild(enemy, "Enemy Leg FR");
        leg.GetComponent<Transform>().LocalPosition = {-0.6f, 0.0f, -0.6f};
        leg.AddComponent<MeshRenderer>().MeshPtr = legMeshMirrored.get();
    }

    {
        Entity leg = scene.CreateChild(enemy, "Enemy Leg BR");
        auto& tc = leg.GetComponent<Transform>();
        tc.LocalPosition = {-0.6f, 0.0f, 0.6f};
        tc.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                              glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)) *
                              glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
        leg.AddComponent<MeshRenderer>().MeshPtr = legMeshMirrored.get();
    }

    auto& collider = enemy.AddComponent<SphereCollider>();
    collider.Radius = 0.5f;
    auto& rb = enemy.AddComponent<Rigidbody>();
    enemy.AddComponent<Movement>();
    rb.Velocity = {0.0f, 0.0f, 0.0f};
    rb.IsStatic = false;
}
