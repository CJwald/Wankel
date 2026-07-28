#pragma once
#include "VertexBufferLayout.h"

namespace Wankel {

class VertexBuffer {
public:
    VertexBuffer(const void* data, unsigned int size);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    void Bind() const;
    void SetLayout(const VertexBufferLayout& layout);
    const VertexBufferLayout& GetLayout() const { return m_Layout; }

    // Respecifies this buffer's storage in place (same GL object ID) - lets a caller reuse an
    // existing VertexBuffer/VAO for entirely new data instead of constructing a new one; any
    // glVertexAttribPointer bindings already made against this ID remain valid (OpenGL binds
    // attributes to a buffer's ID, not a snapshot of its contents), as long as the layout shape
    // (types/counts/offsets/stride) doesn't change between calls.
    void SetData(const void* data, unsigned int size);

private:
    unsigned int m_ID;
    VertexBufferLayout m_Layout;
};

} // namespace Wankel
