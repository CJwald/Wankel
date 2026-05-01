#pragma once
#include "VertexBufferLayout.h"
#include <memory>

namespace Wankel {

	class VertexBuffer;
	class IndexBuffer;

	class VertexArray {
	public:
	    VertexArray();
	    ~VertexArray();
	
	    void Bind() const;
	
	    void AddLayout();

		void AddVertexBuffer(const VertexBuffer& vb);
		void SetIndexBuffer(const IndexBuffer& ib);
	
	private:
	    unsigned int m_ID;
		unsigned int m_IndexBufferID = 0;
	};

}
