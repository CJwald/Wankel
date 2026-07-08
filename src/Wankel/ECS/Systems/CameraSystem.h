#pragma once

namespace Wankel {

class Scene;
class Camera;

class CameraSystem {
public:
    void Update(Scene& scene, Camera& camera);
};

} // namespace Wankel
