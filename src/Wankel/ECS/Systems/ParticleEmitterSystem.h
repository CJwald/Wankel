#pragma once

namespace Wankel {

class Scene;
class ParticleSystem;

// Walks every entity with a Transform + ParticleEmitter and feeds spawns into the shared
// ParticleSystem pool - continuous emission from ParticleEffect::SpawnRate plus any queued bursts.
// Does not simulate or render the pool; the owner (a layer) calls ParticleSystem::Simulate/Render.
class ParticleEmitterSystem {
public:
    void Update(Scene& scene, ParticleSystem& particles, float dt);
};

} // namespace Wankel
