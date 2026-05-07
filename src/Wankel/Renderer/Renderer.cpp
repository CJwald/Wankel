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
		glm::vec3 CameraPos;
	};
	
	static RendererData s_Data;

	void Renderer::Init() {
	    glEnable(GL_DEPTH_TEST);
    	glEnable(GL_CULL_FACE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    	//glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glCullFace(GL_BACK);
	}

	void Renderer::BeginScene(const Camera& camera) {
		s_Data.View = camera.GetViewMatrix();
		s_Data.Projection = camera.GetProjectionMatrix();
		s_Data.CameraPos = camera.GetPosition();
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
		shader->SetVec3("u_CameraPos", s_Data.CameraPos);
		//shader->SetVec3("u_FogColor", glm::vec3(0.5f, 0.6f, 0.7f));
		shader->SetVec3("u_FogColor", glm::vec3(0.12f, 0.1f, 0.2f));
		shader->SetFloat("u_FogDensity", 0.01f);
	
    	mesh.Bind();

		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}

	void Renderer::Draw(const Mesh& mesh) {
	    mesh.Bind();
		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}


	void Renderer::Clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}


	void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
	    glViewport(0, 0, width, height);
	}

}
