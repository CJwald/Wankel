#include <Wankel/Core/Application.h>
#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/CameraController.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

using namespace Wankel;

class CubeLayer : public Layer {
public:
	CubeLayer()
		: Layer("Cube"), m_Controller(1280.0f / 720.0f)
	{
		glEnable(GL_DEPTH_TEST);

		// Lock cursor (one-time setup)
		GLFWwindow* window = static_cast<GLFWwindow*>(
			Application::Get().GetWindow().GetNativeWindow());

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		// Cube vertices
		float vertices[] = {
			// positions
			-0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
			 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,

			-0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
			 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,

			-0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
			-0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

			 0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
			 0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,

			-0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
			 0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,

			-0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
			 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
		};

		glGenVertexArrays(1, &m_VAO);
		glBindVertexArray(m_VAO);

		glGenBuffers(1, &m_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(0);

		// Inline shaders (no file headaches)
		const std::string vertexSrc = R"(
			#version 330 core
			layout(location = 0) in vec3 aPos;

			uniform mat4 model;
			uniform mat4 view;
			uniform mat4 projection;

			void main()
			{
				gl_Position = projection * view * model * vec4(aPos, 1.0);
			}
		)";

		const std::string fragmentSrc = R"(
			#version 330 core
			out vec4 FragColor;

			void main()
			{
				FragColor = vec4(0.6, 1.0, 0.0, 1.0);
			}
		)";

		m_Shader = new Shader(vertexSrc, fragmentSrc);

		// Start camera slightly back so cube is visible
		m_Controller.GetCamera().SetPosition({0.0f, 0.0f, 3.0f});
	}

	void OnUpdate() override
	{
		float time = glfwGetTime();
		float deltaTime = time - m_LastFrame;
		m_LastFrame = time;

		m_Controller.OnUpdate(deltaTime);

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto& cam = m_Controller.GetCamera();

		glm::mat4 model = glm::mat4(1.0f);

		m_Shader->Bind();
		m_Shader->SetMat4("model", model);
		m_Shader->SetMat4("view", cam.GetViewMatrix());
		m_Shader->SetMat4("projection", cam.GetProjectionMatrix());

		glBindVertexArray(m_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

private:
	unsigned int m_VAO, m_VBO;
	Shader* m_Shader;

	CameraController m_Controller;

	float m_LastFrame = 0.0f;
};

class CubeApp : public Application {
public:
	CubeApp()
	{
		PushLayer(new CubeLayer());
	}
};

Application* Wankel::CreateApplication()
{
	return new CubeApp();
}


#include <Wankel/Core/EntryPoint.h>

