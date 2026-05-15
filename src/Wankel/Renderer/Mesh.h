#pragma once

#include <memory>

namespace Wankel {

	class VertexArray;
	class VertexBuffer;
	class IndexBuffer;
	
	class Mesh {
	public:
	    Mesh(const void* vertices, uint32_t size, const uint32_t* indices, uint32_t indexCount);
	    ~Mesh();
	
	    void Bind() const;
	
		uint32_t GetIndexCount() const;
	
	private:
	    std::unique_ptr<VertexArray> m_VertexArray;
	    std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		uint32_t m_IndexCount = 0;
	};

}
