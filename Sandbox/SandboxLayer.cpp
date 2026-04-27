#include "SandboxLayer.h"
#include "cube.h"

#include <Wankel/Core/Application.h>
#include <Wankel/ECS/Components.h>

// To-be removed eventually
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Wankel {

SandboxLayer::SandboxLayer()
	: Layer("Cube"), m_Controller(1280.0f / 720.0f)
{
	glEnable(GL_DEPTH_TEST);

	// Lock cursor
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Viewport fix
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);

	// VAO/VBO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::CubeVertices), Geometry::CubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Shader
	const std::string vertexSrc = R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;

		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		void main() {
			gl_Position = projection * view * model * vec4(aPos, 1.0);
		}
	)";

	const std::string fragmentSrc = R"(
		#version 330 core
		out vec4 FragColor;

		void main() {
			FragColor = vec4(0.6, 1.0, 0.0, 1.0);
		}
	)";

	m_Shader = new Shader(vertexSrc, fragmentSrc);

	// Camera
	m_Controller.GetCamera().SetPosition({0.0f, 0.0f, 3.0f});

	// ECS
	auto cube = m_Scene.CreateEntity();
	auto& transform = cube.AddComponent<TransformComponent>();
	transform.Position = {0.0f, 0.0f, 0.0f};
	transform.Scale = {1.0f, 1.0f, 1.0f};
}

void SandboxLayer::OnUpdate()
{
	float time = glfwGetTime();
	float deltaTime = time - m_LastFrame;
	m_LastFrame = time;

	m_Controller.OnUpdate(deltaTime);

	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto& cam = m_Controller.GetCamera();

	m_Shader->Bind();
	m_Shader->SetMat4("view", cam.GetViewMatrix());
	m_Shader->SetMat4("projection", cam.GetProjectionMatrix());

	glBindVertexArray(m_VAO);

	auto view = m_Scene.Registry().view<TransformComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, transform.Position);
		model = glm::scale(model, transform.Scale);

		m_Shader->SetMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

}
