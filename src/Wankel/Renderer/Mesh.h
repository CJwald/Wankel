#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

namespace Wankel {

class VertexArray;
class VertexBuffer;
class IndexBuffer;

struct Vertex {
    glm::vec3 Position {0.0f};
    glm::vec4 Color {1.0f};
    // Appended (not inserted between Position/Color) so existing 2-element
    // aggregate-init call sites (e.g. Geometry::CubeVertices) keep compiling
    // unchanged and just pick up this default via C++ aggregate-init rules.
    glm::vec3 Normal {0.0f, 1.0f, 0.0f};
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    // Quantizes vertex positions to 16 bits/axis (normalized against quantizeMin/quantizeMax)
    // instead of a full float vec3 - used for voxel terrain chunks. The range must be supplied
    // externally (e.g. a chunk's known [0, ChunkDim*VoxelSize] extent) rather than computed from
    // this mesh's own vertex data: two adjacent chunks share exactly-matching boundary vertices
    // (guaranteed bit-identical in float space by the density-field apron overlap) that must
    // quantize to the SAME integers on both sides, which only holds if every chunk quantizes
    // against the same fixed range - each chunk's own tight vertex AABB would generally differ
    // from its neighbor's (their triangulated surfaces don't usually span the same sub-volume
    // even though they must agree exactly at the shared boundary), reintroducing seam cracks.
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::vec3& quantizeMin,
         const glm::vec3& quantizeMax);

    ~Mesh();

    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    // Respecifies this mesh's GPU buffers in place (same VAO/VBO/IBO GL objects) instead of
    // building a new Mesh - lets a caller (e.g. VoxelWorld's chunk-mesh pool) reuse a retired
    // Mesh's GL objects for entirely new geometry, avoiding a destroy+recreate cycle. Only valid
    // to call on a Mesh originally built via the quantizing constructor above - reusing this on a
    // Mesh built via the plain constructor would leave its VAO's attribute layout (float position,
    // differently typed) mismatched with the quantized data this writes.
    void UpdateData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                    const glm::vec3& quantizeMin, const glm::vec3& quantizeMax);

    void Bind() const;

    uint32_t GetIndexCount() const;
    std::unique_ptr<Mesh> CreateMirrored(bool mirrorX, bool mirrorY, bool mirrorZ) const;

    // Renderer::Submit/SubmitInstanced fold these into the model matrix so a quantized position
    // (already unpacked to [0,1] by GL's normalized-attribute read) lands back at its true local
    // value with no vertex shader changes needed - identity ({0,0,0}/{1,1,1}) for the plain ctor.
    glm::vec3 GetQuantizeMin() const { return m_QuantizeMin; }
    glm::vec3 GetQuantizeExtent() const { return m_QuantizeExtent; }

private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::unique_ptr<VertexBuffer> m_VertexBuffer;
    std::unique_ptr<IndexBuffer> m_IndexBuffer;
    uint32_t m_IndexCount = 0;

    glm::vec3 m_QuantizeMin {0.0f};
    glm::vec3 m_QuantizeExtent {1.0f};
};

} // namespace Wankel
