#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"

namespace Wankel {

Mesh::Mesh(const void* vertices, uint32_t size, uint32_t vertexCount)
    : m_VertexCount(vertexCount)
{
    m_VertexArray = std::make_unique<VertexArray>();
    m_VertexBuffer = std::make_unique<VertexBuffer>(vertices, size);

    m_VertexArray->Bind();
    m_VertexBuffer->Bind();

    // your existing cube layout function
    m_VertexArray->AddLayout();
}

Mesh::~Mesh() {}

void Mesh::Bind() const {
    m_VertexArray->Bind();
    m_VertexBuffer->Bind();
}

}
