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

	private:
	    unsigned int m_ID;
		VertexBufferLayout m_Layout;
	};

}
