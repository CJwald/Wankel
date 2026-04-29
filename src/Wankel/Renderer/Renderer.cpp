#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"

#include <glad/gl.h>
#include <glm/glm.h>

namespace Wankel {

	struct RendererData {
	    glm::mat4 ViewProjection;
	};
	
	static RendererData s_Data;

	void Renderer::Init() {
	    glEnable(GL_DEPTH_TEST);
    	glEnable(GL_CULL_FACE);
    	glCullFace(GL_BACK);
	}

	void Renderer::BeginScene(const Camera& camera) {
	    s_Data.ViewProjection = camera.GetProjectionMatrix() * camera.GetViewMatrix();
		// This should eventually change to Camera::GetViewProjection()
	}

	void Renderer::EndScene() {
	    // Currently nothing (reserved for batching / flush systems later)
	}

	void Renderer::Submit(const glm::mat4& transform, VertexArray* vao, Shader* shader) {
	    shader->Bind();
	    shader->SetMat4("model", transform);
	
	    vao->Bind();
	    glDrawArrays(GL_TRIANGLES, 0, 36); // Temporary Cube assumption:
		                                   //  This should become VertexBuffer + IndexBuffer
		                                   //  or Mesh::GetIndexCount()
	}



	void Renderer::Clear() {
    	glClear(GL_COLOR_BUFFER_BIT);
	}

	void Renderer::Draw(VertexArray* vao, int count) {
		vao->Bind();
		glDrawArrays(GL_TRIANGLES, 0, count);
	}

}
