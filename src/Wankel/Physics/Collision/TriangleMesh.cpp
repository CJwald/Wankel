#include "wkpch.h"
#include "TriangleMesh.h"

namespace Wankel {

TriangleMesh::TriangleMesh(std::vector<glm::vec3> positions, std::vector<uint32_t> indices)
    : m_Positions(std::move(positions)), m_Indices(std::move(indices)) {
    if (m_Positions.empty()) {
        m_LocalBounds = AABB {glm::vec3(0.0f), glm::vec3(0.0f)};
        return;
    }

    glm::vec3 min = m_Positions[0];
    glm::vec3 max = m_Positions[0];

    for (const auto& p : m_Positions) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    m_LocalBounds = AABB {min, max};
}

void TriangleMesh::GetTriangle(size_t triIndex, glm::vec3& a, glm::vec3& b, glm::vec3& c) const {
    a = m_Positions[m_Indices[triIndex * 3 + 0]];
    b = m_Positions[m_Indices[triIndex * 3 + 1]];
    c = m_Positions[m_Indices[triIndex * 3 + 2]];
}

} // namespace Wankel
