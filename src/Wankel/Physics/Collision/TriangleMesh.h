#pragma once

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

#include "BroadPhase/AABB.h"

namespace Wankel {

// CPU-only triangle buffer for collision - no GL resources (unlike Mesh,
// which eagerly creates a VAO/VBO/IBO). Flat positions+indices input
// matches what a future MarchingCubes::Generate (Terrain/MarchingCubes.h,
// not yet implemented) would produce, without coupling to that type.
// A uniform bin grid over LocalBounds (built once, in the ctor) lets
// narrow-phase/raycast code visit only the triangles near a query box
// instead of the whole mesh - LocalBounds() is still the O(1) whole-mesh reject.
class TriangleMesh {
public:
    TriangleMesh(std::vector<glm::vec3> positions, std::vector<uint32_t> indices);

    size_t GetTriangleCount() const { return m_Indices.size() / 3; }
    void GetTriangle(size_t triIndex, glm::vec3& a, glm::vec3& b, glm::vec3& c) const;

    const AABB& LocalBounds() const { return m_LocalBounds; }

    // Appends (to a caller-cleared `out`) every triangle index whose bounds overlap localBox, given in
    // this mesh's local space - deduped, ascending. Superset of the triangles actually touching the box.
    void QueryTriangles(const AABB& localBox, std::vector<uint32_t>& out) const;

private:
    void BuildGrid();
    glm::ivec3 CellOf(const glm::vec3& localPoint) const;

    std::vector<glm::vec3> m_Positions;
    std::vector<uint32_t> m_Indices;
    AABB m_LocalBounds;

    glm::ivec3 m_GridDims {0};
    glm::vec3 m_InvCellSize {0.0f};
    std::vector<uint32_t> m_CellStart;     // CSR offsets into m_CellTriangles, one per cell plus a terminator
    std::vector<uint32_t> m_CellTriangles; // triangle indices, grouped by cell
};

} // namespace Wankel
