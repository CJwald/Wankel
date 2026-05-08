#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Wankel/Core/Time.h"

#include <glad/gl.h>

namespace Wankel {

	struct RendererData {
    	glm::mat4 View;
    	glm::mat4 Projection;
		glm::vec3 CameraPos;

		FogSettings Fog;
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
		shader->SetVec3("u_FogColor", s_Data.Fog.Color);
		shader->SetFloat("u_FogDensity", s_Data.Fog.Density);
		shader->SetFloat("u_Time", Time::GetTime());

		shader->SetFloat("u_FogNoiseScale", s_Data.Fog.NoiseScale);
		shader->SetFloat("u_FogNoiseStrength", s_Data.Fog.NoiseStrength);
		shader->SetFloat("u_FogNoiseSpeed", s_Data.Fog.NoiseSpeed);
		shader->SetInt("u_FogNoiseOctaves", s_Data.Fog.NoiseOctaves);
		shader->SetInt("u_FogNoiseEnabled",
		               s_Data.Fog.NoiseEnabled ? 1 : 0);
		shader->SetVec3("u_FogWindDir", s_Data.Fog.WindDir);
		shader->SetFloat("u_FogWindSpeed", s_Data.Fog.WindSpeed);
	
    	mesh.Bind();

		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}

	void Renderer::Draw(const Mesh& mesh) {
	    mesh.Bind();
		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}


	void Renderer::Clear() {
		const auto& fog = s_Data.Fog;

	    glClearColor(fog.Color.r, fog.Color.g, fog.Color.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}


	void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
	    glViewport(0, 0, width, height);
	}

	void Renderer::SetFog(const FogSettings& fog) {
    	s_Data.Fog = fog;
	}

}
