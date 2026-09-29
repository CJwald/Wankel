#pragma once

#include "ByteRangeAllocator.h"
#include "ChunkGeometryPool.h" // ChunkTransformGPU/DrawElementsIndirectCommand/ChunkInstanceEntry
#include "QuantizedVertex.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Wankel {

// Same role as ChunkGeometryPool, for the CPU voxel color split-sharpness path's heavier
// SplitQuantizedVertex format (one extra baked ColorOther attribute per vertex). A concrete
// sibling class rather than templatizing ChunkGeometryPool: keeps that class (and every existing
// chunk draw depending on it) completely untouched, so the zero-cost-when-split-mode-is-off
// guarantee holds by construction - this class is only ever constructed when CPU split mode is
// actually selected (see VoxelWorld::EnsureSplitGeometryPool).
class SplitChunkGeometryPool {
public:
    SplitChunkGeometryPool(size_t vertexCapacityBytes, size_t indexCapacityBytes, uint32_t maxChunks);
    ~SplitChunkGeometryPool();

    SplitChunkGeometryPool(const SplitChunkGeometryPool&) = delete;
    SplitChunkGeometryPool& operator=(const SplitChunkGeometryPool&) = delete;

    ChunkGeometryHandle Allocate(uint32_t vertexCount, uint32_t indexCount);
    void Free(const ChunkGeometryHandle& handle);

    void Write(const ChunkGeometryHandle& handle, const SplitQuantizedVertex* vertices, uint32_t vertexCount,
              const uint32_t* indices, uint32_t indexCount, const glm::mat4& model, const glm::mat4& normalMatrix);

    void SetTransform(uint32_t chunkSlot, const glm::mat4& model, const glm::mat4& normalMatrix);

    void UploadFrameData(const std::vector<DrawElementsIndirectCommand>& commands,
                        const std::vector<ChunkInstanceEntry>& instances);

    uint32_t GetLastUploadedCommandCount() const { return m_LastCommandCount; }
    uint32_t GetLiveChunkCount() const { return m_LiveChunks; }

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
    ByteRangeAllocator m_SlotAllocator;
};

} // namespace Wankel
