#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include "VertexBufferLayout.h"

namespace Wankel {

	Mesh::Mesh(const void* vertices, uint32_t size,
	           const uint32_t* indices, uint32_t indexCount)
	    : m_IndexCount(indexCount)
	{
	    m_VertexArray = std::make_unique<VertexArray>();
	    m_VertexArray->Bind();
	
	    m_VertexBuffer = std::make_unique<VertexBuffer>(vertices, size);
	    m_IndexBuffer = std::make_unique<IndexBuffer>(indices, indexCount);
	
	    VertexBufferLayout layout;
	    layout.PushFloat(3, "a_Position");
	    layout.PushFloat(4, "a_Color");
	
	    m_VertexBuffer->SetLayout(layout);
	
	    m_VertexBuffer->Bind();
	    m_VertexArray->AddVertexBuffer(*m_VertexBuffer);
	
	    // This is what attaches EBO to VAO
	    m_VertexArray->SetIndexBuffer(*m_IndexBuffer);
	}
	
	Mesh::~Mesh() {}
	
	void Mesh::Bind() const {
	    m_VertexArray->Bind();
	}
	
	uint32_t Mesh::GetIndexCount() const {
	    return m_IndexBuffer->GetCount();
	}

}
