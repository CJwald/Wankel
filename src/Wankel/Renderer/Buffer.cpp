#include "Buffer.h"
#include "VertexBufferLayout.h"
#include <glad/gl.h>

namespace Wankel {

VertexBuffer::VertexBuffer(const void* data, unsigned int size) {
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void VertexBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
}

void VertexBuffer::SetLayout(const VertexBufferLayout& layout) {
    m_Layout = layout;
}

void VertexBuffer::SetData(const void* data, unsigned int size) {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    // DYNAMIC (not STATIC like the one-time constructor upload above) - SetData exists specifically
    // for repeatedly-respecified buffers (see VoxelWorld::m_MeshPool's chunk mesh reuse via
    // Mesh::UpdateData), so the usage hint should match how the buffer is actually used.
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

} // namespace Wankel
