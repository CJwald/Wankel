

#include "Renderer.h"
#include "VertexArray.h"

#include <glad/gl.h>

namespace Wankel {

	void Renderer::Clear() {
    	glClear(GL_COLOR_BUFFER_BIT);
	}

	void Renderer::Draw(VertexArray* vao, int count) {
		vao->Bind();
		glDrawArrays(GL_TRIANGLES, 0, count);
	}

}
