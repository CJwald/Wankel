#include "PhysicsSystem.h"

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/AABBCollision.h"

#include <Wankel/ECS/Components.h>
#include <Wankel/ECS/Scene.h>

#include <glm/glm.hpp>

namespace Wankel {

void PhysicsSystem::Update(Scene& scene, float dt)
{
    auto& registry = scene.Registry();

    // =========================
    // INTEGRATE VELOCITY
    // =========================
    {
        auto view = registry.view<TransformComponent, RigidbodyComponent>();

        for (auto entity : view)
        {
            auto& t = view.get<TransformComponent>(entity);
            auto& rb = view.get<RigidbodyComponent>(entity);

            if (rb.IsStatic)
                continue;

            t.Position += rb.Velocity * dt;
        }
    }

    // =========================
    // COLLISION RESOLUTION
    // =========================
    auto view = registry.view<TransformComponent, AABBComponent, RigidbodyComponent>();

    const float slop = 0.01f;

    for (auto itA = view.begin(); itA != view.end(); ++itA)
    {
        auto a = *itA;

        auto& ta = registry.get<TransformComponent>(a);
        auto& ca = registry.get<AABBComponent>(a);
        auto& rba = registry.get<RigidbodyComponent>(a);

        AABB aabbA = AABB::FromCenterHalfSize(ta.Position, ca.HalfSize);

        auto itB = itA;
        ++itB;

        for (; itB != view.end(); ++itB)
        {
            auto b = *itB;

            auto& tb = registry.get<TransformComponent>(b);
            auto& cb = registry.get<AABBComponent>(b);
            auto& rbb = registry.get<RigidbodyComponent>(b);

            AABB aabbB = AABB::FromCenterHalfSize(tb.Position, cb.HalfSize);

            auto manifold = AABBvsAABB(aabbA, aabbB);

            if (!manifold.Colliding)
                continue;

            // =========================
            // POSITION CORRECTION
            // =========================
            glm::vec3 correction =
                manifold.Normal *
                std::max(manifold.Penetration - slop, 0.0f);

            if (rba.IsStatic && rbb.IsStatic)
                continue;

            if (rba.IsStatic)
            {
                tb.Position += correction;
            }
            else if (rbb.IsStatic)
            {
                ta.Position -= correction;
            }
            else
            {
                ta.Position -= correction * 0.5f;
                tb.Position += correction * 0.5f;
            }

            // =========================
            // VELOCITY RESPONSE (IMPULSE STYLE)
            // =========================
            glm::vec3 relativeVel = rba.Velocity - rbb.Velocity;
            float velAlongNormal = glm::dot(relativeVel, manifold.Normal);

            if (velAlongNormal < 0.0f)
            {
                glm::vec3 impulse = manifold.Normal * velAlongNormal;

                if (!rba.IsStatic)
                    rba.Velocity -= impulse;

                if (!rbb.IsStatic)
                    rbb.Velocity += impulse;
            }
        }
    }
}

}
