#include "wkpch.h"
#include "SplitChunkGeometryPool.h"

#include "VertexArray.h"

#include <glad/gl.h>

namespace Wankel {

SplitChunkGeometryPool::SplitChunkGeometryPool(size_t vertexCapacityBytes, size_t indexCapacityBytes,
                                               uint32_t maxChunks)
    : m_MaxChunks(maxChunks), m_MaxIndirectCommands(maxChunks), m_MaxIndirectInstances(maxChunks * 8),
      m_VertexAllocator(vertexCapacityBytes), m_IndexAllocator(indexCapacityBytes), m_SlotAllocator(maxChunks) {
    glGenVertexArrays(1, &m_VAO);
    VertexArray::BindID(m_VAO);

    // COMBINED VERTEX BUFFER - SplitQuantizedVertex layout (locations 0-2 match ChunkGeometryPool's
    // QuantizedVertex layout exactly, plus location 5 for the extra ColorOther attribute - see that
    // struct's own comment).
    glGenBuffers(1, &m_VertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexCapacityBytes, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(SplitQuantizedVertex),
                          (void*)offsetof(SplitQuantizedVertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SplitQuantizedVertex),
                          (void*)offsetof(SplitQuantizedVertex, Color));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(SplitQuantizedVertex),
                          (void*)offsetof(SplitQuantizedVertex, PackedNormal));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SplitQuantizedVertex),
                          (void*)offsetof(SplitQuantizedVertex, ColorOther));

    // COMBINED PER-INSTANCE BUFFER (locations 3-4) - identical role/layout to ChunkGeometryPool's,
    // see that class for the full comment.
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)m_MaxIndirectInstances * sizeof(ChunkInstanceEntry)), nullptr,
                GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkInstanceEntry),
                          (void*)offsetof(ChunkInstanceEntry, WorldOffset));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(ChunkInstanceEntry),
                           (void*)offsetof(ChunkInstanceEntry, ChunkIndex));
    glVertexAttribDivisor(4, 1);

    glGenBuffers(1, &m_IndexIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCapacityBytes, nullptr, GL_DYNAMIC_DRAW);

    VertexArray::BindID(0);

    glGenBuffers(1, &m_TransformSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)((size_t)maxChunks * sizeof(ChunkTransformGPU)), nullptr,
                GL_DYNAMIC_DRAW);

    glGenBuffers(1, &m_IndirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                (GLsizeiptr)((size_t)m_MaxIndirectCommands * sizeof(DrawElementsIndirectCommand)), nullptr,
                GL_DYNAMIC_DRAW);
}

SplitChunkGeometryPool::~SplitChunkGeometryPool() {
    glDeleteBuffers(1, &m_IndirectBuffer);
    glDeleteBuffers(1, &m_TransformSSBO);
    glDeleteBuffers(1, &m_IndexIBO);
    glDeleteBuffers(1, &m_InstanceVBO);
    glDeleteBuffers(1, &m_VertexVBO);
    glDeleteVertexArrays(1, &m_VAO);
}

ChunkGeometryHandle SplitChunkGeometryPool::Allocate(uint32_t vertexCount, uint32_t indexCount) {
    ChunkGeometryHandle handle;

    size_t vertexBytes = (size_t)vertexCount * sizeof(SplitQuantizedVertex);
    size_t indexBytes = (size_t)indexCount * sizeof(uint32_t);

    size_t vertexOffset = m_VertexAllocator.Alloc(vertexBytes);
    if (vertexOffset == ByteRangeAllocator::kFailed)
        return handle;

    size_t indexOffset = m_IndexAllocator.Alloc(indexBytes);
    if (indexOffset == ByteRangeAllocator::kFailed) {
        m_VertexAllocator.Free(vertexOffset, vertexBytes);
        return handle;
    }

    size_t slot = m_SlotAllocator.Alloc(1);
    if (slot == ByteRangeAllocator::kFailed) {
        m_VertexAllocator.Free(vertexOffset, vertexBytes);
        m_IndexAllocator.Free(indexOffset, indexBytes);
        return handle;
    }

    handle.Valid = true;
    handle.VertexBase = (uint32_t)(vertexOffset / sizeof(SplitQuantizedVertex));
    handle.VertexCount = vertexCount;
    handle.FirstIndex = (uint32_t)(indexOffset / sizeof(uint32_t));
    handle.IndexCount = indexCount;
    handle.ChunkSlot = (uint32_t)slot;
    m_LiveChunks++;
    return handle;
}

void SplitChunkGeometryPool::Free(const ChunkGeometryHandle& handle) {
    if (!handle.Valid)
        return;

    m_VertexAllocator.Free((size_t)handle.VertexBase * sizeof(SplitQuantizedVertex),
                           (size_t)handle.VertexCount * sizeof(SplitQuantizedVertex));
    m_IndexAllocator.Free((size_t)handle.FirstIndex * sizeof(uint32_t), (size_t)handle.IndexCount * sizeof(uint32_t));
    m_SlotAllocator.Free(handle.ChunkSlot, 1);
    m_LiveChunks--;
}

void SplitChunkGeometryPool::Write(const ChunkGeometryHandle& handle, const SplitQuantizedVertex* vertices,
                                   uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                                   const glm::mat4& model, const glm::mat4& normalMatrix) {
    if (!handle.Valid)
        return;

    // See ChunkGeometryPool::Write's own comment on why the VAO must be (re)bound before touching
    // GL_ELEMENT_ARRAY_BUFFER - same reasoning applies here.
    VertexArray::BindID(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)((size_t)handle.VertexBase * sizeof(SplitQuantizedVertex)),
                    (GLsizeiptr)((size_t)vertexCount * sizeof(SplitQuantizedVertex)), vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexIBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)((size_t)handle.FirstIndex * sizeof(uint32_t)),
                    (GLsizeiptr)((size_t)indexCount * sizeof(uint32_t)), indices);

    SetTransform(handle.ChunkSlot, model, normalMatrix);
}

void SplitChunkGeometryPool::SetTransform(uint32_t chunkSlot, const glm::mat4& model, const glm::mat4& normalMatrix) {
    ChunkTransformGPU t {model, normalMatrix};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TransformSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)((size_t)chunkSlot * sizeof(ChunkTransformGPU)),
                    sizeof(ChunkTransformGPU), &t);
}

void SplitChunkGeometryPool::UploadFrameData(const std::vector<DrawElementsIndirectCommand>& commands,
                                             const std::vector<ChunkInstanceEntry>& instances) {
    uint32_t commandCount = (uint32_t)commands.size();
    if (commandCount > m_MaxIndirectCommands) {
        WK_CORE_WARNING("SplitChunkGeometryPool::UploadFrameData - {0} commands submitted, truncating to capacity ({1})",
                        commandCount, m_MaxIndirectCommands);
        commandCount = m_MaxIndirectCommands;
    }

    uint32_t instanceCount = (uint32_t)instances.size();
    if (instanceCount > m_MaxIndirectInstances) {
        WK_CORE_WARNING("SplitChunkGeometryPool::UploadFrameData - {0} instances submitted, truncating to capacity ({1})",
                        instanceCount, m_MaxIndirectInstances);
        instanceCount = m_MaxIndirectInstances;
    }

    if (instanceCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)((size_t)instanceCount * sizeof(ChunkInstanceEntry)),
                        instances.data());
    }

    if (commandCount > 0) {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectBuffer);
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
                        (GLsizeiptr)((size_t)commandCount * sizeof(DrawElementsIndirectCommand)), commands.data());
    }

    m_LastCommandCount = commandCount;
}

void SplitChunkGeometryPool::Bind() const {
    VertexArray::BindID(m_VAO);
}

} // namespace Wankel
