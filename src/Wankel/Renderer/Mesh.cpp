#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include "VertexBufferLayout.h"

namespace Wankel {

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
		: m_Vertices(vertices), m_Indices(indices), m_IndexCount((uint32_t)indices.size()) {

		m_VertexArray = std::make_unique<VertexArray>();
		m_VertexArray->Bind();

		m_VertexBuffer = std::make_unique<VertexBuffer>(
		    m_Vertices.data(),
		    (uint32_t)(m_Vertices.size() * sizeof(Vertex))
		);

		m_IndexBuffer = std::make_unique<IndexBuffer>(
		    m_Indices.data(),
		    m_IndexCount
		);

		VertexBufferLayout layout;
		layout.PushFloat(3, "a_Position");
		layout.PushFloat(4, "a_Color");

		m_VertexBuffer->SetLayout(layout);

		m_VertexBuffer->Bind();
		m_VertexArray->AddVertexBuffer(*m_VertexBuffer);

		m_VertexArray->SetIndexBuffer(*m_IndexBuffer);
	}
	
	Mesh::~Mesh() {}
	
	void Mesh::Bind() const {
	    m_VertexArray->Bind();
	}
	
	uint32_t Mesh::GetIndexCount() const {
	    return m_IndexBuffer->GetCount();
	}

	std::unique_ptr<Mesh> Mesh::CreateMirrored(bool mirrorX, bool mirrorY, bool mirrorZ) const {
		auto vertices = m_Vertices;
		auto indices = m_Indices;

		for (auto& v : vertices) {
		    if (mirrorX) v.Position.x *= -1.0f;
		    if (mirrorY) v.Position.y *= -1.0f;
		    if (mirrorZ) v.Position.z *= -1.0f;
		}

		// Count number of mirrored axes
		int mirrorCount = 0;
		if (mirrorX) mirrorCount++;
		if (mirrorY) mirrorCount++;
		if (mirrorZ) mirrorCount++;

		// Odd number of reflections flips handedness
		if (mirrorCount % 2 == 1) {
		    for (size_t i = 0; i < indices.size(); i += 3) {
		        std::swap(indices[i + 1], indices[i + 2]);
		    }
		}

		return std::make_unique<Mesh>(vertices, indices);
	}

}
