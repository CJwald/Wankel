#pragma once
#include "VertexBufferLayout.h"
#include <memory>

namespace Wankel {

	class VertexBuffer;

	class VertexArray {
	public:
	    VertexArray();
	    ~VertexArray();
	
	    void Bind() const;
	
	    void AddLayout();

		void AddVertexBuffer(const VertexBuffer& vb);
	
	private:
	    unsigned int m_ID;
	};

}
