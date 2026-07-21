#include "ColliderDebugDraw.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/Renderer/DebugDraw.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Wankel;

void ColliderDebugDraw::Draw(Scene& scene, const glm::vec3& worldOffset) {
    if (!Renderer::DebugEnabled)
        return;

    auto& registry = scene.Registry();

    auto debugView = registry.view<Transform, MeshRenderer>();
    auto colliderView = registry.view<Transform, AABBCollider>();
    auto sphereColliderView = registry.view<Transform, SphereCollider>();

    // DEBUG AXES
    for (auto entity : debugView) {
        auto& transform = debugView.get<Transform>(entity);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform;
        glm::vec3 origin = glm::vec3(model[3]);
        float axisLength = 0.25f;

        glm::vec3 right = glm::normalize(glm::vec3(model[0]));
        glm::vec3 up = glm::normalize(glm::vec3(model[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(model[2]));

        std::vector<DebugLine> lines = {
            {origin, origin + right * axisLength, {0.8, 0.3, 0.0}},  // X axis (Right)
            {origin, origin + up * axisLength, {0.6, 1.0, 0.0}},     // Y axis (Up)
            {origin, origin + forward * axisLength, {0.4, 0.0, 0.9}} // Z axis (Backward)
        };

        // Parent link
        if (registry.all_of<Parent>(entity)) {
            auto parent = registry.get<Parent>(entity).Parent;
            if (parent) {
                auto& childTransform = registry.get<Transform>(entity);
                auto& parentTransform = parent.GetComponent<Transform>();
                glm::vec3 childPos = glm::vec3(childTransform.WorldTransform[3]);
                glm::vec3 parentPos = glm::vec3(parentTransform.WorldTransform[3]);
                lines.push_back({childPos, parentPos, {1, 1, 1}});
            }
        }
        Renderer::SubmitDebugLines(lines);
    }

    // Collider Debug
    for (auto entity : colliderView) {
        auto& transform = colliderView.get<Transform>(entity);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform;
        auto& collider = colliderView.get<AABBCollider>(entity);
        glm::vec3 origin = glm::vec3(model[3]);
        float axisLength = 0.25f;

        glm::vec3 right = glm::normalize(glm::vec3(model[0]));
        glm::vec3 up = glm::normalize(glm::vec3(model[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(model[2]));

        std::vector<DebugLine> lines = {};

        glm::vec3 cDims = collider.HalfSize;
        glm::vec3 cOrigin = glm::vec3(model[3]) + collider.Offset;
        glm::vec3 colliderLineColor = {0.4f, 0.0f, 1.0f};
        // Right
        lines.push_back({cOrigin + right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         cOrigin + right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         cOrigin + right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         cOrigin + right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         cOrigin + right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        // Left
        lines.push_back({cOrigin - right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         cOrigin - right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin - right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         cOrigin - right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin - right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         cOrigin - right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin - right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         cOrigin - right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        // Connection lines
        lines.push_back({cOrigin + right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         cOrigin - right * cDims[0] + up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         cOrigin - right * cDims[0] + up * cDims[1] - forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         cOrigin - right * cDims[0] - up * cDims[1] + forward * cDims[2],
                         colliderLineColor});
        lines.push_back({cOrigin + right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         cOrigin - right * cDims[0] - up * cDims[1] - forward * cDims[2],
                         colliderLineColor});

        Renderer::SubmitDebugLines(lines);
    }

    // Collider Debug (Sphere)
    for (auto entity : sphereColliderView) {
        auto& transform = sphereColliderView.get<Transform>(entity);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform;
        auto& collider = sphereColliderView.get<SphereCollider>(entity);
        glm::vec3 cOrigin = glm::vec3(model[3]) + collider.Offset;
        float r = collider.Radius;
        glm::vec3 colliderLineColor = {0.4f, 0.0f, 1.0f};

        constexpr int kSegments = 24;
        constexpr float kTwoPi = 6.28318530718f;
        std::vector<DebugLine> lines;
        lines.reserve(kSegments * 3);

        auto addCircle = [&](const glm::vec3& axisA, const glm::vec3& axisB) {
            glm::vec3 prev = cOrigin + axisA * r;
            for (int i = 1; i <= kSegments; i++) {
                float theta = (float)i / (float)kSegments * kTwoPi;
                glm::vec3 next = cOrigin + (axisA * cosf(theta) + axisB * sinf(theta)) * r;
                lines.push_back({prev, next, colliderLineColor});
                prev = next;
            }
        };

        addCircle({1, 0, 0}, {0, 1, 0}); // XY plane
        addCircle({1, 0, 0}, {0, 0, 1}); // XZ plane
        addCircle({0, 1, 0}, {0, 0, 1}); // YZ plane

        Renderer::SubmitDebugLines(lines);
    }
}
