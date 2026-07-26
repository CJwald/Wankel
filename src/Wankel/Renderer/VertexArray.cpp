#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include <glad/gl.h>

namespace Wankel {

namespace {
unsigned int s_BoundVAO = 0; // GL is global state - one cache shared by every VertexArray instance
}

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_ID);
}

VertexArray::~VertexArray() {
    if (s_BoundVAO == m_ID)
        s_BoundVAO = 0; // avoid a stale cache hit if a later VAO reuses this GL name
    glDeleteVertexArrays(1, &m_ID);
}

void VertexArray::Bind() const {
    if (s_BoundVAO == m_ID)
        return;
    glBindVertexArray(m_ID);
    s_BoundVAO = m_ID;
}

void VertexArray::AddVertexBuffer(const VertexBuffer& vb) {
    Bind();
    vb.Bind();

    const auto& layout = vb.GetLayout();
    const auto& elements = layout.GetElements();

    for (const auto& e : elements) {
        glEnableVertexAttribArray(m_NextAttribIndex);

        glVertexAttribPointer(m_NextAttribIndex, e.Count, e.Type, e.Normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
                              (const void*)(intptr_t)e.Offset);

        m_NextAttribIndex++;
    }
}

void VertexArray::SetIndexBuffer(const IndexBuffer& ib) {
    Bind();
    ib.Bind();

    m_IndexBufferID = ib.GetID();
}
} // namespace Wankel
