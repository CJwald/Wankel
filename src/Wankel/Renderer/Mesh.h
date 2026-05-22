#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

namespace Wankel {

	class VertexArray;
	class VertexBuffer;
	class IndexBuffer;
	
	struct Vertex {
		glm::vec3 Position;
		glm::vec4 Color;
	};

	class Mesh {
	public:
	    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	    ~Mesh();
	
	    void Bind() const;
	
		uint32_t GetIndexCount() const;
		std::unique_ptr<Mesh> CreateMirrored(bool mirrorX, bool mirrorY, bool mirrorZ) const;
	
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
	    std::unique_ptr<VertexArray> m_VertexArray;
	    std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		uint32_t m_IndexCount = 0;
	};

}
