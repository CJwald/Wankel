#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "VertexBufferLayout.h"

namespace Wankel {

Mesh::Mesh(const void* vertices, uint32_t size, uint32_t vertexCount)
    : m_VertexCount(vertexCount)
{
    m_VertexArray = std::make_unique<VertexArray>();
    m_VertexBuffer = std::make_unique<VertexBuffer>(vertices, size);
	
	VertexBufferLayout layout;

	// cube = position only right now
	layout.PushFloat(3, "a_Position");

	m_VertexBuffer->SetLayout(layout);

	m_VertexArray->Bind();
	m_VertexArray->AddVertexBuffer(*m_VertexBuffer);
}

Mesh::~Mesh() {}

void Mesh::Bind() const {
    m_VertexArray->Bind();
    m_VertexBuffer->Bind();
}

}
