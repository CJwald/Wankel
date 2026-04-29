#include "VertexArray.h"
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
	
	void VertexArray::AddLayout() {
	    glBindVertexArray(m_ID);

	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	    glEnableVertexAttribArray(0);
	}

}
