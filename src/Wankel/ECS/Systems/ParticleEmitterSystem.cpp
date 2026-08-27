#include "wkpch.h"
#include "Wankel/ECS/Systems/ParticleEmitterSystem.h"

#include "Wankel/ECS/Components/ParticleComponents.h"
#include "Wankel/ECS/Components/TransformComponents.h"
#include "Wankel/ECS/Scene.h"
#include "Wankel/Particles/ParticleSystem.h"

#include <glm/glm.hpp>

namespace Wankel {

void ParticleEmitterSystem::Update(Scene& scene, ParticleSystem& particles, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, ParticleEmitter>();

    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& emitter = view.get<ParticleEmitter>(entity);

        glm::vec3 origin = glm::vec3(transform.FinalTransform * glm::vec4(emitter.LocalOffset, 1.0f));

        glm::vec3 axis = glm::mat3(transform.FinalTransform) * emitter.LocalDirection;
        axis = glm::dot(axis, axis) > 1e-8f ? glm::normalize(axis) : glm::vec3(0.0f, 0.0f, -1.0f);

        if (emitter.Enabled && emitter.Effect.SpawnRate > 0.0f) {
            emitter.SpawnAccumulator += emitter.Effect.SpawnRate * dt;
            auto n = (uint32_t)emitter.SpawnAccumulator;
            if (n > 0) {
                emitter.SpawnAccumulator -= (float)n;
                particles.Spawn(emitter.Effect, origin, axis, n);
            }
        }

        if (emitter.PendingBurst > 0) {
            particles.Spawn(emitter.Effect, origin, axis, emitter.PendingBurst);
            emitter.PendingBurst = 0;
        }
    }
}

} // namespace Wankel
