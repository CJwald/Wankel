#pragma once
#include <glm/glm.hpp>

namespace Wankel {

    struct TransformComponent {
        glm::vec3 Position{0.0f};
        glm::vec3 Rotation{0.0f};
        glm::vec3 Scale{1.0f};
    };

    struct CameraComponent {
        float FOV = 45.0f;
        float Near = 0.1f;
        float Far = 1000.0f;
    };

    struct MeshComponent {
        // pointer/handle to your existing mesh
        void* Mesh = nullptr;
    };

}
