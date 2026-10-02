#pragma once

namespace Wankel {

class Scene;
class ParticleSystem;

// Walks every entity with a Transform + ParticleEmitter and feeds spawns into the shared
// ParticleSystem pool - each layer's continuous Emission.Rate plus any queued bursts.
// Does not simulate or render the pool; the owner (a layer) calls ParticleSystem::Simulate/Render.
class ParticleEmitterSystem {
public:
    void Update(Scene& scene, ParticleSystem& particles, float dt);

    // Debug lines (Renderer::SubmitGameplayLines): a small cross at each emitter plus its emit axis.
    static void SubmitDebugLines(Scene& scene);
};

} // namespace Wankel
