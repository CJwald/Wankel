#include "wkpch.h"
#include "ChunkGeometryPool.h"

#include <glad/gl.h>

namespace Wankel {

ChunkGeometryPool::ChunkGeometryPool(size_t vertexCapacityBytes, size_t indexCapacityBytes, uint32_t maxChunks)
    : m_MaxChunks(maxChunks), m_MaxIndirectCommands(maxChunks),
      // Headroom over 1 instance/chunk for tile-repeat copies of the same chunk - matches the
      // spirit of Renderer's own kMaxInstancesPerDraw, just sized for the whole frame's total
      // instead of one chunk's own instance count.
      m_MaxIndirectInstances(maxChunks * 8), m_VertexAllocator(vertexCapacityBytes),
      m_IndexAllocator(indexCapacityBytes), m_SlotAllocator(maxChunks) {
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // COMBINED VERTEX BUFFER - QuantizedVertex layout, must match Mesh's quantizing constructor
    // exactly (locations 0-2) so a chunk's data means the same thing whether it lands here or in a
    // standalone Mesh (pool-exhaustion fallback).
    glGenBuffers(1, &m_VertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexCapacityBytes, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(QuantizedVertex),
                          (void*)offsetof(QuantizedVertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuantizedVertex), (void*)offsetof(QuantizedVertex, Color));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(QuantizedVertex),
                          (void*)offsetof(QuantizedVertex, PackedNormal));

    // COMBINED PER-INSTANCE BUFFER (locations 3-4) - one (WorldOffset, ChunkIndex) pair per
    // surviving instance this frame, rewritten wholesale each frame via UploadFrameData - see
    // chunk.vert's aInstanceOffset/aChunkIndex and this class's own header comment on the "manual
    // chunk index" addressing scheme (avoids needing gl_DrawID/GL 4.6).
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)m_MaxIndirectInstances * sizeof(ChunkInstanceEntry)), nullptr,
                GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ChunkInstanceEntry),
                          (void*)offsetof(ChunkInstanceEntry, WorldOffset));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    // Integer attribute (glVertexAttribIPointer, not the float-converting glVertexAttribPointer) -
    // aChunkIndex is read as a true GLSL uint in chunk.vert, not a normalized/converted float.
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(ChunkInstanceEntry),
                           (void*)offsetof(ChunkInstanceEntry, ChunkIndex));
    glVertexAttribDivisor(4, 1);

    // COMBINED INDEX BUFFER - element array buffer binding is captured into the VAO's own state at
    // bind time, same as the vertex attribute bindings above; no further action needed at draw time
    // beyond binding this VAO.
    glGenBuffers(1, &m_IndexIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCapacityBytes, nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    // PER-CHUNK TRANSFORM SSBO - one ChunkTransformGPU slot per chunk, indexed by aChunkIndex.
    // Written once per chunk on generate/evict (SetTransform), not every frame - chunks are static.
    glGenBuffers(1, &m_TransformSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)((size_t)maxChunks * sizeof(ChunkTransformGPU)), nullptr,
                GL_DYNAMIC_DRAW);

    // PER-FRAME INDIRECT COMMAND BUFFER - one DrawElementsIndirectCommand per visible chunk this
    // frame, rewritten wholesale each frame via UploadFrameData.
    glGenBuffers(1, &m_IndirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                (GLsizeiptr)((size_t)m_MaxIndirectCommands * sizeof(DrawElementsIndirectCommand)), nullptr,
                GL_DYNAMIC_DRAW);
}

ChunkGeometryPool::~ChunkGeometryPool() {
    glDeleteBuffers(1, &m_IndirectBuffer);
    glDeleteBuffers(1, &m_TransformSSBO);
    glDeleteBuffers(1, &m_IndexIBO);
    glDeleteBuffers(1, &m_InstanceVBO);
    glDeleteBuffers(1, &m_VertexVBO);
    glDeleteVertexArrays(1, &m_VAO);
}

ChunkGeometryHandle ChunkGeometryPool::Allocate(uint32_t vertexCount, uint32_t indexCount) {
    ChunkGeometryHandle handle;

    size_t vertexBytes = (size_t)vertexCount * sizeof(QuantizedVertex);
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
    handle.VertexBase = (uint32_t)(vertexOffset / sizeof(QuantizedVertex));
    handle.VertexCount = vertexCount;
    handle.FirstIndex = (uint32_t)(indexOffset / sizeof(uint32_t));
    handle.IndexCount = indexCount;
    handle.ChunkSlot = (uint32_t)slot;
    m_LiveChunks++;
    return handle;
}

void ChunkGeometryPool::Free(const ChunkGeometryHandle& handle) {
    if (!handle.Valid)
        return;

    m_VertexAllocator.Free((size_t)handle.VertexBase * sizeof(QuantizedVertex),
                           (size_t)handle.VertexCount * sizeof(QuantizedVertex));
    m_IndexAllocator.Free((size_t)handle.FirstIndex * sizeof(uint32_t), (size_t)handle.IndexCount * sizeof(uint32_t));
    m_SlotAllocator.Free(handle.ChunkSlot, 1);
    m_LiveChunks--;
}

void ChunkGeometryPool::Write(const ChunkGeometryHandle& handle, const QuantizedVertex* vertices,
                              uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                              const glm::mat4& model, const glm::mat4& normalMatrix) {
    if (!handle.Valid)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)((size_t)handle.VertexBase * sizeof(QuantizedVertex)),
                    (GLsizeiptr)((size_t)vertexCount * sizeof(QuantizedVertex)), vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexIBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)((size_t)handle.FirstIndex * sizeof(uint32_t)),
                    (GLsizeiptr)((size_t)indexCount * sizeof(uint32_t)), indices);

    SetTransform(handle.ChunkSlot, model, normalMatrix);
}

void ChunkGeometryPool::SetTransform(uint32_t chunkSlot, const glm::mat4& model, const glm::mat4& normalMatrix) {
    ChunkTransformGPU t {model, normalMatrix};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TransformSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)((size_t)chunkSlot * sizeof(ChunkTransformGPU)),
                    sizeof(ChunkTransformGPU), &t);
}

void ChunkGeometryPool::UploadFrameData(const std::vector<DrawElementsIndirectCommand>& commands,
                                        const std::vector<ChunkInstanceEntry>& instances) {
    uint32_t commandCount = (uint32_t)commands.size();
    if (commandCount > m_MaxIndirectCommands) {
        WK_CORE_WARNING("ChunkGeometryPool::UploadFrameData - {0} commands submitted, truncating to capacity ({1})",
                        commandCount, m_MaxIndirectCommands);
        commandCount = m_MaxIndirectCommands;
    }

    uint32_t instanceCount = (uint32_t)instances.size();
    if (instanceCount > m_MaxIndirectInstances) {
        WK_CORE_WARNING("ChunkGeometryPool::UploadFrameData - {0} instances submitted, truncating to capacity ({1})",
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

void ChunkGeometryPool::Bind() const {
    glBindVertexArray(m_VAO);
}

void ChunkGeometryPool::DrawOne(const ChunkGeometryHandle& handle, const glm::vec3& instanceOffset) const {
    if (!handle.Valid)
        return;

    // Writes into instance slot 0 and immediately issues a draw reading it - safe because GL command
    // issuance is strictly ordered on this context/thread (same principle Renderer::SubmitInstanced
    // already relies on when it reuses one shared instance buffer across a sequence of per-chunk
    // draws), so there's no risk of a later DrawOne's write clobbering data an earlier draw hasn't
    // consumed yet.
    ChunkInstanceEntry entry {instanceOffset, handle.ChunkSlot};
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ChunkInstanceEntry), &entry);

    Bind();
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, (GLsizei)handle.IndexCount, GL_UNSIGNED_INT,
                                      (void*)((size_t)handle.FirstIndex * sizeof(uint32_t)), 1,
                                      (GLint)handle.VertexBase);
}

} // namespace Wankel
