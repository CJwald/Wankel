#pragma once
#include "VertexBufferLayout.h"
#include <memory>

namespace Wankel {

class VertexBuffer;
class IndexBuffer;

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const;

    // Bind a bare GL VAO name - Renderer's own pass VAOs, ChunkGeometryPool's VAO, an explicit
    // unbind with 0 - while keeping VertexArray's shared bound-VAO cache in sync. EVERY
    // glBindVertexArray in the engine must go through this or Bind(): a raw call leaves the cache
    // stale, and a later Bind() whose id happens to match that stale value then skips its bind and
    // the draw (or a mid-frame Mesh's attribute/index-buffer setup) lands on the wrong VAO.
    static void BindID(unsigned int id);

    void AddLayout();

    void AddVertexBuffer(const VertexBuffer& vb);
    void SetIndexBuffer(const IndexBuffer& ib);

private:
    unsigned int m_ID;
    unsigned int m_IndexBufferID = 0;
    unsigned int m_NextAttribIndex = 0;
};

} // namespace Wankel
