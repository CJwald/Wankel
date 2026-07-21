#include "WorldFactory.h"
#include "../cube.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Assets/AssetManager.h>
#include <Wankel/Renderer/Mesh.h>

#include <glm/gtc/quaternion.hpp>
#include <random>

using namespace Wankel;

namespace {

float RandomFloat() {
    static std::random_device rd;  // seed
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    return dist(gen);
}

} // namespace

void WorldFactory::Create(Scene& scene) {
    Ref<Mesh> cubeMesh = AssetManager::GetOrCreateMesh("builtin:cube", [] { 
		return CreateRef<Mesh>(Geometry::CubeVertices, Geometry::CubeIndices); 
	});
    Ref<Mesh> groundMesh = AssetManager::GetMesh("Assets/Mesh/Rolling200m.ply");

    // RANDOM CUBES
    int numCubes = 20;
    float spawnRange = 10.f;
    for (int i = 0; i < numCubes; i++) {
        Entity e = scene.CreateEntity();
        e.AddComponent<Tag>().Name = "Cube";

        auto& t = e.AddComponent<Transform>();
        float X = RandomFloat() * spawnRange;
        float Y = 20 + RandomFloat() * spawnRange;
        float Z = RandomFloat() * spawnRange;
        t.LocalPosition = {X, Y, Z};
        t.LocalOrientation = glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(1, 0, 0)) *
                             glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0, 1, 0)) *
                             glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0, 0, 1));
        e.AddComponent<MeshRenderer>().MeshPtr = cubeMesh.get();
        e.AddComponent<Material>(Material {{0.9f, 0.9f, 0.92f}, 0.15f, 1.0f, {0.0f, 0.0f, 0.0f}});

        auto& rb = e.AddComponent<Rigidbody>();
        rb.IsStatic = true;

        auto& collider = e.AddComponent<AABBCollider>();
        collider.HalfSize = {0.5f, 0.5f, 0.5f};
    }

    // WORLD
    Entity b = scene.CreateEntity();
    b.AddComponent<Tag>().Name = "World";
    auto& tb = b.AddComponent<Transform>();
    tb.LocalPosition = {0.0f, -25.0f, 0.0f};
    tb.LocalOrientation = glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)) *
                          glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 1, 0)) *
                          glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
    b.AddComponent<MeshRenderer>().MeshPtr = groundMesh.get();
    b.AddComponent<Material>(Material {{0.6f, 0.55f, 0.5f}, 0.85f, 0.0f, {0.02f, 0.015f, 0.0f}});
    auto& rb_box = b.AddComponent<Rigidbody>();
    rb_box.IsStatic = true;
    //auto& worldcollider = b.AddComponent<AABBCollider>();
    //worldcollider.HalfSize = {100.0f, 25.0f, 100.0f}; // There are bugs with large colliders.
}
