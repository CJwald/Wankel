#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "AABB.h"

namespace Wankel {

class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize) : m_CellSize(cellSize) {}

    void Clear() {
        m_Cells.clear();
        m_EntityCells.clear();
    }

    void Insert(entt::entity entity, const glm::vec3& position) {
        auto key = Hash(PositionToCell(position));
        m_Cells[key].push_back(entity);
        m_EntityCells[entity].push_back(key);
    }

    // Inserts into every cell `bounds` spans, not just one - a point-sized
    // Insert() would make a large collider (e.g. a terrain mesh chunk)
    // undiscoverable from most nearby dynamic bodies, since Query() only
    // scans a fixed 3x3x3 neighborhood around a single point.
    void InsertAABB(entt::entity entity, const AABB& bounds) {
        glm::ivec3 minCell = PositionToCell(bounds.Min);
        glm::ivec3 maxCell = PositionToCell(bounds.Max);

        for (int x = minCell.x; x <= maxCell.x; x++)
            for (int y = minCell.y; y <= maxCell.y; y++)
                for (int z = minCell.z; z <= maxCell.z; z++) {
                    auto key = Hash(glm::ivec3(x, y, z));
                    m_Cells[key].push_back(entity);
                    m_EntityCells[entity].push_back(key);
                }
    }

    // Removes entity from every cell it currently occupies, via the reverse map Insert/InsertAABB
    // populate above - lets a caller update a single collider (e.g. one edited voxel terrain chunk)
    // without a full Clear()+rebuild of the whole grid. No-op if entity was never inserted.
    void Remove(entt::entity entity) {
        auto it = m_EntityCells.find(entity);
        if (it == m_EntityCells.end())
            return;

        for (int64_t key : it->second) {
            auto cellIt = m_Cells.find(key);
            if (cellIt == m_Cells.end())
                continue;
            auto& bucket = cellIt->second;
            bucket.erase(std::remove(bucket.begin(), bucket.end(), entity), bucket.end());
            if (bucket.empty())
                m_Cells.erase(cellIt);
        }
        m_EntityCells.erase(it);
    }

    std::vector<entt::entity> Query(const glm::vec3& position) {
        std::vector<entt::entity> result;

        glm::ivec3 cell = PositionToCell(position);

        for (int x = -1; x <= 1; x++)
            for (int y = -1; y <= 1; y++)
                for (int z = -1; z <= 1; z++) {
                    glm::ivec3 neighbor = cell + glm::ivec3(x, y, z);
                    auto key = Hash(neighbor);

                    auto it = m_Cells.find(key);
                    if (it == m_Cells.end())
                        continue;

                    auto& bucket = it->second;
                    result.insert(result.end(), bucket.begin(), bucket.end());
                }

        return result;
    }

private:
    float m_CellSize;

    std::unordered_map<int64_t, std::vector<entt::entity>> m_Cells;
    std::unordered_map<entt::entity, std::vector<int64_t>> m_EntityCells;

private:
    glm::ivec3 PositionToCell(const glm::vec3& pos) {
        return glm::ivec3(floor(pos.x / m_CellSize), floor(pos.y / m_CellSize), floor(pos.z / m_CellSize));
    }

    int64_t Hash(const glm::ivec3& cell) {
        return ((int64_t)cell.x * 73856093) ^ ((int64_t)cell.y * 19349663) ^ ((int64_t)cell.z * 83492791);
    }
};

} // namespace Wankel
