
#pragma once

namespace Wankel {

class Scene;

class TransformSystem {
public:
    void Update(Scene& scene);
    void UpdateFinalTransforms(Scene& scene);
};

} // namespace Wankel
