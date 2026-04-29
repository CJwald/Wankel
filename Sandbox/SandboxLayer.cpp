#include "SandboxLayer.h"
#include "cube.h"

#include <Wankel/Core/Application.h>
#include <Wankel/Core/Time.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/Mesh.h>

// To-be removed eventually
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Wankel {

SandboxLayer::SandboxLayer()
	: Layer("Cube"), m_Controller(1280.0f / 720.0f)
{

	// Mesh setup
	m_CubeMesh = std::make_unique<Mesh>(
        Geometry::CubeVertices,
        sizeof(Geometry::CubeVertices),
        36
    );

	// Shader
	m_Shader = std::make_unique<Shader>(
		"shaders/cube.vert", 
		"shaders/cube.frag"
	);

	// Camera
	m_Controller.GetCamera().SetPosition({0.0f, 0.0f, 3.0f});

	// ECS
	auto cube = m_Scene.CreateEntity();
	auto& transform = cube.AddComponent<TransformComponent>();
	transform.Position = {0.0f, 0.0f, 0.0f};
	transform.Scale = {1.0f, 1.0f, 1.0f};
}

void SandboxLayer::OnUpdate() {
	float time = Time::GetTime();
	float dt = time - m_LastFrame;
	m_LastFrame = time;

	m_Controller.OnUpdate(dt);

	Renderer::Clear(0.1f, 0.1f, 0.1f, 1.0f);

	auto& cam = m_Controller.GetCamera();

	Renderer::BeginScene(cam);

	auto view = m_Scene.Registry().view<TransformComponent>();

	for (auto entity : view) {
		auto& transform = view.get<TransformComponent>(entity);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, transform.Position);
		model = glm::scale(model, transform.Scale);

		Renderer::Submit(model, *m_CubeMesh, m_Shader.get());
	}
	
	Renderer::EndScene();
}

}
