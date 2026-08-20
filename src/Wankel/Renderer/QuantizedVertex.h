#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace Wankel {

// Packs a normalized vec3 into GL_INT_2_10_10_10_REV layout (X:bits0-9, Y:bits10-19, Z:bits20-29,
// W:bits30-31, each a signed 10-bit component) - GL unpacks this to a normalized vec4 in the
// shader for free, no decode ALU cost, at 4 bytes instead of a plain vec3's 12. Shared by Mesh's
// quantizing constructor and ChunkGeometryPool so both paths produce byte-identical GPU data.
inline uint32_t PackNormal(const glm::vec3& n) {
    glm::vec3 c = glm::clamp(n, -1.0f, 1.0f);
    auto packComponent = [](float v) -> uint32_t {
        return (uint32_t)(int32_t)std::lround(v * 511.0f) & 0x3FF; // 10-bit signed, masked
    };
    return packComponent(c.x) | (packComponent(c.y) << 10) | (packComponent(c.z) << 20);
}

// Same as PackedVertex (Mesh.cpp's plain-mesh format) but with Position also quantized. Packed to
// 1-byte alignment so VertexBufferLayout's manually tracked stride matches this struct's true byte
// layout exactly - see the full rationale (including the AMD Mesa unaligned-fetch bug this avoids)
// on QuantizedVertex in Mesh.cpp before this was extracted here.
#pragma pack(push, 1)
struct QuantizedVertex {
    uint16_t Position[3];  // offset 0, 6 bytes
    uint16_t Padding = 0;  // offset 6, 2 bytes - pushes Color to offset 8 (4-byte aligned)
    glm::vec4 Color;       // offset 8, 16 bytes
    uint32_t PackedNormal; // offset 24, 4 bytes (4-byte aligned)
};
#pragma pack(pop)
static_assert(sizeof(QuantizedVertex) == 28, "QuantizedVertex must be tightly packed - GPU stride depends on this");

inline uint16_t QuantizeComponent(float value, float min, float extent) {
    float t = extent > 1e-8f ? glm::clamp((value - min) / extent, 0.0f, 1.0f) : 0.0f;
    return (uint16_t)std::lround(t * 65535.0f);
}

// Shared by Mesh's quantizing constructor/UpdateData and ChunkGeometryPool, so a chunk's GPU data
// is byte-identical whether it lands in a standalone Mesh (pool-exhaustion fallback) or the shared
// combined buffer.
inline std::vector<QuantizedVertex> BuildQuantizedVertices(const std::vector<Vertex>& vertices, const glm::vec3& min,
                                                            const glm::vec3& extent) {
    std::vector<QuantizedVertex> packed;
    packed.reserve(vertices.size());
    for (const Vertex& v : vertices) {
        QuantizedVertex qv;
        qv.Position[0] = QuantizeComponent(v.Position.x, min.x, extent.x);
        qv.Position[1] = QuantizeComponent(v.Position.y, min.y, extent.y);
        qv.Position[2] = QuantizeComponent(v.Position.z, min.z, extent.z);
        qv.Color = v.Color;
        qv.PackedNormal = PackNormal(v.Normal);
        packed.push_back(qv);
    }
    return packed;
}

} // namespace Wankel
