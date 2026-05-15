#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Buffer.h"

#include "Wankel/Core/Time.h"

#include <glad/gl.h>

namespace Wankel {


	struct DebugVertex {
	    glm::vec3 Position;
	    glm::vec3 Color;
	};


	bool Renderer::DebugEnabled = false;


	struct RendererData {
    	glm::mat4 View;
    	glm::mat4 Projection;
		glm::vec3 CameraPos;

		FogSettings Fog;

		std::vector<DebugVertex> DebugVertices;
		uint32_t DebugVAO = 0;
		uint32_t DebugVBO = 0;
		Shader* DebugShader = nullptr;
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

		// DEBUG PASS GPU OBJECTS
		glGenVertexArrays(1, &s_Data.DebugVAO);
		glGenBuffers(1, &s_Data.DebugVBO);
		glBindVertexArray(s_Data.DebugVAO);
		glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * 65536, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3,	GL_FLOAT, GL_FALSE,	sizeof(DebugVertex), (void*)offsetof(DebugVertex, Position));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3,	GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, Color));
		s_Data.DebugShader = new Shader(
			"WankelShaders/debug.vert",
			"WankelShaders/debug.frag"
		);
	}


	void Renderer::Shutdown() {
		delete s_Data.DebugShader;
		glDeleteBuffers(1, &s_Data.DebugVBO);
		glDeleteVertexArrays(1, &s_Data.DebugVAO);
	}


	void Renderer::BeginScene(const Camera& camera) {
		s_Data.View = camera.GetViewMatrix();
		s_Data.Projection = camera.GetProjectionMatrix();
		s_Data.CameraPos = camera.GetPosition();
		s_Data.DebugVertices.clear();
	}


	void Renderer::EndScene() {
		// DEBUG PASS
		if (DebugEnabled && !s_Data.DebugVertices.empty()) {
			s_Data.DebugShader->Bind();
			s_Data.DebugShader->SetMat4("view", s_Data.View);
			s_Data.DebugShader->SetMat4("projection", s_Data.Projection);

			glBindVertexArray(s_Data.DebugVAO);
			glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, s_Data.DebugVertices.size()	* sizeof(DebugVertex), s_Data.DebugVertices.data());
			glDisable(GL_CULL_FACE);
			glDrawArrays(GL_LINES, 0, (GLsizei)s_Data.DebugVertices.size());
			glEnable(GL_CULL_FACE);
		}
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
		shader->SetInt("u_FogNoiseOctaves", s_Data.Fog.NoiseOctaves);
		shader->SetInt("u_FogNoiseEnabled", s_Data.Fog.NoiseEnabled ? 1 : 0);
		shader->SetVec3("u_FogWindDir", s_Data.Fog.WindDir);
		shader->SetFloat("u_FogWindSpeed", s_Data.Fog.WindSpeed);
	
    	mesh.Bind();

		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}


	void Renderer::SubmitDebugLines(const std::vector<DebugLine>& lines) {
		if (!DebugEnabled)
			return;

		for (const auto& line : lines) {
			s_Data.DebugVertices.push_back({ line.P0, line.Color });
			s_Data.DebugVertices.push_back({ line.P1, line.Color });
		}
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
