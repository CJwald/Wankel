#include "IndexBuffer.h"
#include <glad/gl.h>

namespace Wankel {

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count)
    : m_Count(count)
{
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void IndexBuffer::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
}

uint32_t IndexBuffer::GetCount() const {
    return m_Count;
}

uint32_t IndexBuffer::GetID() const {
    return m_ID;
}

}
