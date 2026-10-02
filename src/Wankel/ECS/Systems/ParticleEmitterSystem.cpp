#include "wkpch.h"
#include "Wankel/ECS/Systems/ParticleEmitterSystem.h"

#include "Wankel/ECS/Components/ParticleComponents.h"
#include "Wankel/ECS/Components/TransformComponents.h"
#include "Wankel/ECS/Scene.h"
#include "Wankel/Particles/ParticleSystem.h"
#include "Wankel/Renderer/Renderer.h"

#include <glm/glm.hpp>

namespace Wankel {

void ParticleEmitterSystem::Update(Scene& scene, ParticleSystem& particles, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, ParticleEmitter>();

    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& emitter = view.get<ParticleEmitter>(entity);
        if (!emitter.Effect)
            continue;

        glm::vec3 origin = glm::vec3(transform.FinalTransform * glm::vec4(emitter.LocalOffset, 1.0f));

        glm::vec3 axis = glm::mat3(transform.FinalTransform) * emitter.LocalDirection;
        axis = glm::dot(axis, axis) > 1e-8f ? glm::normalize(axis) : glm::vec3(0.0f, 0.0f, -1.0f);

        glm::vec3 velocity(0.0f);
        if (emitter.HasPreviousOrigin && dt > 0.0f)
            velocity = (origin - emitter.PreviousOrigin) / dt;
        emitter.PreviousOrigin = origin;
        emitter.HasPreviousOrigin = true;

        const auto& layers = emitter.Effect->Layers;
        emitter.SpawnAccumulators.resize(layers.size(), 0.0f);
        if (emitter.Enabled) {
            for (uint32_t i = 0; i < layers.size(); i++) {
                if (!layers[i].Enabled || layers[i].Emission.Rate <= 0.0f)
                    continue;
                float& acc = emitter.SpawnAccumulators[i];
                acc += layers[i].Emission.Rate * dt;
                auto n = (uint32_t)acc;
                if (n > 0) {
                    acc -= (float)n;
                    particles.Emit(emitter.Effect, i, origin, axis, velocity, n);
                }
            }
        }

        if (emitter.PendingBursts > 0) {
            particles.Play(emitter.Effect, origin, axis, velocity, emitter.PendingBursts);
            emitter.PendingBursts = 0;
        }
    }
}

void ParticleEmitterSystem::SubmitDebugLines(Scene& scene) {
    std::vector<DebugLine> lines;
    auto view = scene.Registry().view<Transform, ParticleEmitter>();
    for (auto entity : view) {
        const auto& transform = view.get<Transform>(entity);
        const auto& emitter = view.get<ParticleEmitter>(entity);
        glm::vec3 origin = glm::vec3(transform.FinalTransform * glm::vec4(emitter.LocalOffset, 1.0f));
        glm::vec3 axis = glm::mat3(transform.FinalTransform) * emitter.LocalDirection;
        axis = glm::dot(axis, axis) > 1e-8f ? glm::normalize(axis) : glm::vec3(0.0f, 0.0f, -1.0f);

        const glm::vec3 color(0.2f, 1.0f, 0.4f);
        constexpr float kCross = 0.05f;
        lines.push_back({origin - glm::vec3(kCross, 0, 0), origin + glm::vec3(kCross, 0, 0), color});
        lines.push_back({origin - glm::vec3(0, kCross, 0), origin + glm::vec3(0, kCross, 0), color});
        lines.push_back({origin - glm::vec3(0, 0, kCross), origin + glm::vec3(0, 0, kCross), color});
        lines.push_back({origin, origin + axis * 0.5f, color});
    }
    Renderer::SubmitGameplayLines(lines);
}

} // namespace Wankel
