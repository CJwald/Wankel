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
// No BVH - brute-force per-triangle iteration is the narrow-phase's job,
// justified by expected voxel-chunk-sized triangle counts (low thousands
// at most); LocalBounds() gives narrow-phase functions an O(1) whole-mesh
// reject before any per-triangle work.
class TriangleMesh {
public:
    TriangleMesh(std::vector<glm::vec3> positions, std::vector<uint32_t> indices);

    size_t GetTriangleCount() const { return m_Indices.size() / 3; }
    void GetTriangle(size_t triIndex, glm::vec3& a, glm::vec3& b, glm::vec3& c) const;

    const AABB& LocalBounds() const { return m_LocalBounds; }

private:
    std::vector<glm::vec3> m_Positions;
    std::vector<uint32_t> m_Indices;
    AABB m_LocalBounds;
};

} // namespace Wankel
