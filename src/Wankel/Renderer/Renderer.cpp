#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"

#include <glad/gl.h>

namespace Wankel {

	struct RendererData {
    	glm::mat4 View;
    	glm::mat4 Projection;
	};
	
	static RendererData s_Data;

	void Renderer::Init() {
	    glEnable(GL_DEPTH_TEST);
    	glEnable(GL_CULL_FACE);
    	glCullFace(GL_BACK);
	}

	void Renderer::BeginScene(const Camera& camera) {
		s_Data.View = camera.GetViewMatrix();
		s_Data.Projection = camera.GetProjectionMatrix();
		// This should eventually change to Camera::GetViewProjection()
	}

	void Renderer::EndScene() {
	    // Currently nothing (reserved for batching / flush systems later)
	}

	void Renderer::Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader) {
	    shader->Bind();
		shader->SetMat4("view", s_Data.View);
		shader->SetMat4("projection", s_Data.Projection);
	    shader->SetMat4("model", transform);
	
    	mesh.Bind();

    	glDrawArrays(GL_TRIANGLES, 0, mesh.GetVertexCount());
	}

	void Renderer::Draw(const Mesh& mesh) {
	    mesh.Bind();
	    glDrawArrays(GL_TRIANGLES, 0, mesh.GetVertexCount());
	}


	void Renderer::Clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

}
