#include "wkpch.h"
#include "MeshUtils.h"

#include "Wankel/Renderer/Mesh.h"

namespace Wankel {

void ComputeSmoothNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    for (auto& v : vertices)
        v.Normal = glm::vec3(0.0f);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t ia = indices[i];
        uint32_t ib = indices[i + 1];
        uint32_t ic = indices[i + 2];

        const glm::vec3& pa = vertices[ia].Position;
        const glm::vec3& pb = vertices[ib].Position;
        const glm::vec3& pc = vertices[ic].Position;

        // Unnormalized cross product's magnitude is proportional to the
        // triangle's area, so summing it directly area-weights the
        // contribution to each shared vertex before the final normalize.
        glm::vec3 faceNormal = glm::cross(pb - pa, pc - pa);

        vertices[ia].Normal += faceNormal;
        vertices[ib].Normal += faceNormal;
        vertices[ic].Normal += faceNormal;
    }

    for (auto& v : vertices) {
        float len = glm::length(v.Normal);
        v.Normal = len > 1e-8f ? v.Normal / len : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace Wankel
