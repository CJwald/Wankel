#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Wankel/Renderer/Mesh.h"
#include "Wankel/Renderer/Renderer.h" // pulls in Wankel::Material, reused as-is as an ECS component
#include "Wankel/ECS/Entity.h"

#include <cstdint>


namespace Wankel {

struct MeshRenderer {
    Mesh* MeshPtr = nullptr;

    glm::vec3 LocalPosition {0.0f};
    glm::quat LocalRotation {1, 0, 0, 0};
    glm::vec3 LocalScale {1.0f};

    bool MirrorX = false;
    bool MirrorY = false;
    bool MirrorZ = false;

    glm::vec3 RotationPivot {0.0f};

    // Matched against a camera's CameraComponent::CullingMask - rendered only if (Layers & mask) != 0.
    // All bits by default (visible to any camera); narrow this only for meshes that must be excluded
    // from specific cameras (e.g. a first-person viewmodel that only its own camera should see).
    uint32_t Layers = 0xFFFFFFFFu;

    glm::mat4 GetLocalTransform() const {
        glm::mat4 pivotToOrigin = glm::translate(glm::mat4(1.0f), -RotationPivot);
        glm::mat4 pivotBack = glm::translate(glm::mat4(1.0f), RotationPivot);
        glm::vec3 finalScale = LocalScale;
        if (MirrorX)
            finalScale.x *= -1.0f;
        if (MirrorY)
            finalScale.y *= -1.0f;
        if (MirrorZ)
            finalScale.z *= -1.0f;
        return glm::translate(glm::mat4(1.0f), LocalPosition) * pivotBack * glm::toMat4(LocalRotation) * pivotToOrigin *
               glm::scale(glm::mat4(1.0f), finalScale);
    }
};


struct CameraComponent {
    float FOV = 66.0f; // Vertical FOV ~= 100 Horizontal on 16:9
    float Near = 0.1f;
    float Far = 1000.0f;

    bool Primary = true;

    // Which MeshRenderer::Layers this camera can see - a mesh renders only if (its Layers & this) !=
    // 0. All bits by default (sees everything); narrow explicitly per camera, the same way Unity/
    // Unreal culling masks work - a new camera that doesn't care about this keeps seeing everything.
    uint32_t CullingMask = 0xFFFFFFFFu;
};

} // namespace Wankel
