#include "wkpch.h"
#include "TriangleMesh.h"

#include <algorithm>
#include <cmath>

namespace Wankel {

namespace {

// Bins along the mesh's longest axis - a voxel chunk (16 m cube) gets 2 m bins, a few dozen triangles each.
constexpr int kGridResolution = 8;

} // namespace

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
    BuildGrid();
}

void TriangleMesh::GetTriangle(size_t triIndex, glm::vec3& a, glm::vec3& b, glm::vec3& c) const {
    a = m_Positions[m_Indices[triIndex * 3 + 0]];
    b = m_Positions[m_Indices[triIndex * 3 + 1]];
    c = m_Positions[m_Indices[triIndex * 3 + 2]];
}

glm::ivec3 TriangleMesh::CellOf(const glm::vec3& localPoint) const {
    glm::ivec3 cell = glm::ivec3(glm::floor((localPoint - m_LocalBounds.Min) * m_InvCellSize));
    return glm::clamp(cell, glm::ivec3(0), m_GridDims - 1);
}

void TriangleMesh::BuildGrid() {
    size_t triCount = GetTriangleCount();
    if (triCount == 0)
        return;

    glm::vec3 extent = m_LocalBounds.Max - m_LocalBounds.Min;
    float longest = std::max({extent.x, extent.y, extent.z});
    if (longest <= 0.0f)
        return; // degenerate (all points coincide) - QueryTriangles falls back to every triangle

    for (int axis = 0; axis < 3; axis++) {
        m_GridDims[axis] = std::clamp((int)std::ceil(kGridResolution * extent[axis] / longest), 1, kGridResolution);
        m_InvCellSize[axis] = extent[axis] > 0.0f ? (float)m_GridDims[axis] / extent[axis] : 0.0f;
    }

    size_t cellCount = (size_t)m_GridDims.x * m_GridDims.y * m_GridDims.z;
    auto cellIndex = [&](int x, int y, int z) {
        return ((size_t)z * m_GridDims.y + y) * m_GridDims.x + x;
    };

    // Two passes (count, then fill) into CSR arrays - one allocation each instead of a vector per cell.
    auto forEachCellOfTriangle = [&](size_t tri, auto&& fn) {
        glm::vec3 a, b, c;
        GetTriangle(tri, a, b, c);
        glm::ivec3 lo = CellOf(glm::min(a, glm::min(b, c)));
        glm::ivec3 hi = CellOf(glm::max(a, glm::max(b, c)));
        for (int z = lo.z; z <= hi.z; z++)
            for (int y = lo.y; y <= hi.y; y++)
                for (int x = lo.x; x <= hi.x; x++)
                    fn(cellIndex(x, y, z));
    };

    m_CellStart.assign(cellCount + 1, 0);
    for (size_t tri = 0; tri < triCount; tri++)
        forEachCellOfTriangle(tri, [&](size_t cell) { m_CellStart[cell + 1]++; });
    for (size_t cell = 0; cell < cellCount; cell++)
        m_CellStart[cell + 1] += m_CellStart[cell];

    m_CellTriangles.resize(m_CellStart[cellCount]);
    std::vector<uint32_t> cursor(m_CellStart.begin(), m_CellStart.end() - 1);
    for (size_t tri = 0; tri < triCount; tri++)
        forEachCellOfTriangle(tri, [&](size_t cell) { m_CellTriangles[cursor[cell]++] = (uint32_t)tri; });
}

void TriangleMesh::QueryTriangles(const AABB& localBox, std::vector<uint32_t>& out) const {
    if (!localBox.Intersects(m_LocalBounds))
        return;

    size_t first = out.size();
    if (m_CellStart.empty()) {
        for (size_t tri = 0; tri < GetTriangleCount(); tri++)
            out.push_back((uint32_t)tri);
        return;
    }

    glm::ivec3 lo = CellOf(localBox.Min);
    glm::ivec3 hi = CellOf(localBox.Max);
    for (int z = lo.z; z <= hi.z; z++) {
        for (int y = lo.y; y <= hi.y; y++) {
            for (int x = lo.x; x <= hi.x; x++) {
                size_t cell = ((size_t)z * m_GridDims.y + y) * m_GridDims.x + x;
                out.insert(out.end(), m_CellTriangles.begin() + m_CellStart[cell],
                           m_CellTriangles.begin() + m_CellStart[cell + 1]);
            }
        }
    }

    // A triangle spanning several bins was appended once per bin.
    if (lo != hi) {
        std::sort(out.begin() + (std::ptrdiff_t)first, out.end());
        out.erase(std::unique(out.begin() + (std::ptrdiff_t)first, out.end()), out.end());
    }
}

} // namespace Wankel
