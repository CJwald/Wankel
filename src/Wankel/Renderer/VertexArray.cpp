#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include <glad/gl.h>

namespace Wankel {

	VertexArray::VertexArray() {
	    glGenVertexArrays(1, &m_ID);
	}
	
	VertexArray::~VertexArray() {
	    glDeleteVertexArrays(1, &m_ID);
	}
	
	void VertexArray::Bind() const {
	    glBindVertexArray(m_ID);
	}
	
	void VertexArray::AddVertexBuffer(const VertexBuffer& vb) {
		glBindVertexArray(m_ID);
		vb.Bind();
	
		const auto& layout = vb.GetLayout();
		const auto& elements = layout.GetElements();
	
		uint32_t index = 0;
	
		for (const auto& e : elements) {
			glEnableVertexAttribArray(index);
	
			glVertexAttribPointer(
				index,
				e.Count,
				e.Type,
				e.Normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				(const void*)(intptr_t)e.Offset
			);
	
			index++;
		}
	}

	void VertexArray::SetIndexBuffer(const IndexBuffer& ib) {
    	glBindVertexArray(m_ID);
    	ib.Bind();

    	m_IndexBufferID = ib.GetID();
	}
}
