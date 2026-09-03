#pragma once

#include "ByteRangeAllocator.h"
#include "QuantizedVertex.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Wankel {

// Layout must exactly match chunk.vert's std430 `ChunkDataBlock.Chunks[]` struct. NormalMatrix is
// a full mat4 (only the upper-left 3x3 used) rather than mat3, to sidestep std430's awkward mat3
// column-padding rules.
struct ChunkTransformGPU {
    glm::mat4 Model {1.0f};
    glm::mat4 NormalMatrix {1.0f};
};

// Matches the GL-mandated memory layout for one glMultiDrawElementsIndirect command (the
// "DrawElementsIndirectCommand" struct in the OpenGL spec) - field order/sizes are load-bearing,
// not just a convenient grouping.
struct DrawElementsIndirectCommand {
    uint32_t Count = 0;
    uint32_t InstanceCount = 0;
    uint32_t FirstIndex = 0;
    int32_t BaseVertex = 0;
    uint32_t BaseInstance = 0;
};

// One (world offset, chunk index) pair per surviving (chunk x tile-repeat instance) this frame -
// see chunk.vert's aInstanceOffset/aChunkIndex attributes. WorldOffset first/ChunkIndex second so
// this is layout-compatible with a plain vec3-offsets buffer plus a trailing index, matching the
// two vertex attributes read from it.
struct ChunkInstanceEntry {
    glm::vec3 WorldOffset {0.0f};
    uint32_t ChunkIndex = 0;
};

// A chunk's sub-allocation within the pool's combined buffers - opaque to the caller beyond what's
// needed to build a DrawElementsIndirectCommand once the chunk is known visible, and to Free() it
// later.
struct ChunkGeometryHandle {
    bool Valid = false;
    uint32_t VertexBase = 0; // first vertex (baseVertex) within the combined vertex buffer
    uint32_t VertexCount = 0;
    uint32_t FirstIndex = 0; // first index (firstIndex) within the combined index buffer
    uint32_t IndexCount = 0;
    uint32_t ChunkSlot = 0; // this chunk's index into the transform SSBO's array
};

class Shader;
struct Material;

// Combined vertex/index/transform storage for many voxel chunks, batched into a single
// glMultiDrawElementsIndirect call per frame instead of one Submit()/SubmitInstanced per chunk -
// see docs/TODO.md's "Open" item on cross-chunk draw-call batching. Vertex data must already be in
// QuantizedVertex GPU format - this class is otherwise format-agnostic, it has no knowledge of
// voxels/marching cubes.
//
// Sub-allocation only, no growth: if the pool runs out of vertex/index/slot capacity, Allocate()
// returns an invalid handle and the caller should fall back to a standalone Wankel::Mesh for that
// one chunk instead - simpler and safer than buffer growth/compaction for a first version.
class ChunkGeometryPool {
public:
    ChunkGeometryPool(size_t vertexCapacityBytes, size_t indexCapacityBytes, uint32_t maxChunks);
    ~ChunkGeometryPool();

    ChunkGeometryPool(const ChunkGeometryPool&) = delete;
    ChunkGeometryPool& operator=(const ChunkGeometryPool&) = delete;

    // Returns an invalid handle (Valid=false) if there's no single free block large enough for this
    // many vertices/indices, or no free chunk slot.
    ChunkGeometryHandle Allocate(uint32_t vertexCount, uint32_t indexCount);

    // Releases a chunk's vertex/index/SSBO-slot allocations back to the pool's free lists. No-op on
    // an invalid handle.
    void Free(const ChunkGeometryHandle& handle);

    // Writes this chunk's geometry into its already-allocated sub-range (a targeted glBufferSubData,
    // never a full-buffer orphan) and its transform into the SSBO slot. vertexCount/indexCount must
    // match what Allocate() was called with for this handle.
    void Write(const ChunkGeometryHandle& handle, const QuantizedVertex* vertices, uint32_t vertexCount,
              const uint32_t* indices, uint32_t indexCount, const glm::mat4& model, const glm::mat4& normalMatrix);

    // Updates just a chunk's transform without touching its geometry - not currently used (chunks
    // are static once created) but kept separate from Write for clarity/future use.
    void SetTransform(uint32_t chunkSlot, const glm::mat4& model, const glm::mat4& normalMatrix);

    // Uploads this frame's visible-chunk draw commands + per-instance data (built by the caller from
    // its own frustum/distance culling - this class has no visibility logic). Call once per frame
    // before Renderer::SubmitIndirect; truncates and logs a warning if either array exceeds this
    // pool's configured per-frame capacity.
    void UploadFrameData(const std::vector<DrawElementsIndirectCommand>& commands,
                        const std::vector<ChunkInstanceEntry>& instances);

    uint32_t GetLastUploadedCommandCount() const { return m_LastCommandCount; }
    uint32_t GetLiveChunkCount() const { return m_LiveChunks; }

    // For Renderer::SubmitIndirect - binds the VAO wired to this pool's combined vertex/index/
    // instance buffers.
    void Bind() const;
    uint32_t GetTransformSSBO() const { return m_TransformSSBO; }
    uint32_t GetIndirectBuffer() const { return m_IndirectBuffer; }

private:
    uint32_t m_VAO = 0;
    uint32_t m_VertexVBO = 0;
    uint32_t m_IndexIBO = 0;
    uint32_t m_InstanceVBO = 0;
    uint32_t m_TransformSSBO = 0;
    uint32_t m_IndirectBuffer = 0;

    uint32_t m_MaxChunks = 0;
    uint32_t m_LiveChunks = 0;
    uint32_t m_MaxIndirectCommands = 0;
    uint32_t m_MaxIndirectInstances = 0;
    uint32_t m_LastCommandCount = 0;

    ByteRangeAllocator m_VertexAllocator;
    ByteRangeAllocator m_IndexAllocator;
    ByteRangeAllocator m_SlotAllocator; // units of 1 chunk slot each, not bytes
};

} // namespace Wankel
