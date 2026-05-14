#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Wankel {

class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize) : m_CellSize(cellSize) {}

    void Clear() {
        m_Cells.clear();
    }

    void Insert(entt::entity entity, const glm::vec3& position) {
        auto key = Hash(PositionToCell(position));
        m_Cells[key].push_back(entity);
    }

    std::vector<entt::entity> Query(const glm::vec3& position) {
        std::vector<entt::entity> result;
        glm::ivec3 cell = PositionToCell(position);

        // check neighboring cells too
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        for (int z = -1; z <= 1; z++) {
            glm::ivec3 neighbor = cell + glm::ivec3(x,y,z);
            auto key = Hash(neighbor);

            if (m_Cells.find(key) != m_Cells.end()) {
                auto& bucket = m_Cells[key];
                result.insert(result.end(), bucket.begin(), bucket.end());
            }
        }

        return result;
    }

private:
    float m_CellSize;
    std::unordered_map<int64_t, std::vector<entt::entity>> m_Cells;

    glm::ivec3 PositionToCell(const glm::vec3& pos) {
        return glm::ivec3(
            floor(pos.x / m_CellSize),
            floor(pos.y / m_CellSize),
            floor(pos.z / m_CellSize)
        );
    }

    int64_t Hash(const glm::ivec3& cell) {
        // big primes hashing
        return (int64_t)(cell.x * 73856093) ^
               (int64_t)(cell.y * 19349663) ^
               (int64_t)(cell.z * 83492791);
    }
};

}
