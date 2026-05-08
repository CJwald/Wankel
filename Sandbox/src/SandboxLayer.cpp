#include "SandboxLayer.h"
#include "cube.h"
#include "triangle.h"
#include "PLYLoader.h"

#include <Wankel/Core/Application.h>
#include <Wankel/Core/Time.h>
#include <Wankel/Core/Events/Event.h>
#include "Wankel/Core/Input.h"
#include "Wankel/Core/ControllerInput.h"
#include "Wankel/Core/KeyCodes.h"
#include <Wankel/ECS/Components.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/Mesh.h>
#include <Wankel/Core/Window.h>

#include <imgui.h>
#include <GLFW/glfw3.h>

// To-be removed eventually
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

float RandomFloat() {
    static std::random_device rd;  // seed
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    return dist(gen);
}

namespace Wankel {

SandboxLayer::SandboxLayer()
	: Layer("Cube"), m_Controller(1280.0f / 720.0f)
{

	//auto meshData = PLYLoader::Load("Assets/Mesh/ShipGrey.ply");
	auto meshData = PLYLoader::Load("Assets/Mesh/Ship02.ply");
	std::cout << "Verts: " << meshData.Vertices.size() << std::endl;
	std::cout << "Indices: " << meshData.Indices.size() << std::endl;
	m_ShipMesh = std::make_unique<Mesh>(
	    meshData.Vertices.data(),
	    meshData.Vertices.size() * sizeof(float),
	    meshData.Indices.data(),
	    (uint32_t)meshData.Indices.size()
	);

	// Mesh setup
	m_CubeMesh = std::make_unique<Mesh>(
	    Geometry::CubeVertices,
	    sizeof(Geometry::CubeVertices),
	    Geometry::CubeIndices,
		sizeof(Geometry::CubeIndices) / sizeof(uint32_t)
	);
	//m_TriangleMesh = std::make_unique<Mesh>(
	//    Geometry::TriangleVertices,
	//    sizeof(Geometry::TriangleVertices),
	//    Geometry::TriangleIndices,
	//    sizeof(Geometry::TriangleIndices)
	//);

	// Shader
	m_Shader = std::make_unique<Shader>(
		"shaders/cube.vert", 
		"shaders/cube.frag"
	);

	// =========================
    // PLAYER ENTITY
    // =========================
    auto player = m_Scene.CreateEntity();
	auto& pt = player.AddComponent<TransformComponent>();
    pt.Position = {0,0,0};

    player.AddComponent<MeshComponent>().MeshPtr = m_ShipMesh.get();
    player.AddComponent<PlayerControllerComponent>();

    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
    camEntity.AddComponent<TransformComponent>();
    auto& follow = camEntity.AddComponent<FollowCameraComponent>();

    follow.Target = player;
    follow.Offset = {-0.0f, 0.32f, 0.2f};
	float roll = 0.0f; float pitch = -4.0f; float yaw = 0.0f; //-1.0f; 
	follow.RotationOffset =
    	glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(yaw), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(roll), glm::vec3(0,0,1));

	m_DebugFollow = &follow;

	// Collider
	auto& collider = player.AddComponent<AABBComponent>();
	collider.HalfSize = {0.5f, 0.5f, 0.5f};
	auto& rb = player.AddComponent<RigidbodyComponent>();
	rb.Velocity = {0,0,0};
	rb.IsStatic = false;	


	// =========================
    // OTHER OBJECTS
    // =========================
	int numCubes = 1000;
	float spawnRange = 100.f;
    for (int i = 0; i < numCubes; i++) {
        auto e = m_Scene.CreateEntity();

        auto& t = e.AddComponent<TransformComponent>();
		float X = RandomFloat() * spawnRange;
		float Y = RandomFloat() * spawnRange;
		float Z = RandomFloat() * spawnRange;
        t.Position = {X, Y, Z};
        t.Orientation = 
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(1,0,0)) *
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0,1,0)) *
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0,0,1));
		e.AddComponent<MeshComponent>().MeshPtr = m_CubeMesh.get();

        auto& rb = e.AddComponent<RigidbodyComponent>();
        rb.IsStatic = true;
	
        // Collider
        auto& collider = e.AddComponent<AABBComponent>();
        collider.HalfSize = {0.5f, 0.5f, 0.5f};
    }

	// =========================
	// DEFAULT FOG
	// =========================
	m_Fog.Color = {0.12f, 0.1f, 0.2f};
	m_Fog.Density = 0.01f;

	// Lock mouse initially
	auto& window = Application::Get().GetWindow();

	GLFWwindow* glfwWindow =
	    static_cast<GLFWwindow*>(window.GetNativeWindow());

	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}


void SandboxLayer::OnUpdate() {
	float time = Time::GetTime();
	float dt = time - m_LastFrame;
	m_LastFrame = time;

	// ADD ESC to switch to ImGui
	static bool escPressedLastFrame = false;
	bool escPressed = Input::IsKeyPressed(Key::Escape);
	if (escPressed && !escPressedLastFrame) {
		m_GameFocused = !m_GameFocused;
		auto& window = Application::Get().GetWindow();
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window.GetNativeWindow());
		if (m_GameFocused) {
			glfwSetInputMode(
			    glfwWindow,
			    GLFW_CURSOR,
			    GLFW_CURSOR_DISABLED
			);
		} else {
			glfwSetInputMode(
			    glfwWindow,
			    GLFW_CURSOR,
			    GLFW_CURSOR_NORMAL
			);
		}
	}
	escPressedLastFrame = escPressed;
	m_PlayerInputSystem.Update(
	    m_Scene,
	    dt,
	    m_GameFocused
	);

    m_Scene.OnUpdate(dt, m_Controller.GetCamera());
	
	Renderer::SetFog(m_Fog);

    Renderer::BeginScene(m_Controller.GetCamera());

	auto view = m_Scene.Registry().view<TransformComponent, MeshComponent>();

	for (auto entity : view) {
		auto& transform = view.get<TransformComponent>(entity);
        auto& mesh = view.get<MeshComponent>(entity);

		glm::mat4 model = transform.GetTransform();

        Renderer::Submit(model, *mesh.MeshPtr, m_Shader.get());
	}

	Renderer::EndScene();
}

void SandboxLayer::OnImGuiRender() {
	if (!m_GameFocused) {
		ImGui::Begin("Renderer Debug");

		
		// =========================
		// FOG
		// =========================
		ImGui::Text("Fog");

		ImGui::Text("Fog Color");
		ImGui::SameLine();
		ImGui::ColorPicker3("##FogColor", 
			&m_Fog.Color[0], 
			ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float
		);

		ImGui::ColorEdit3(
		    "Fog Color",
		    &m_Fog.Color[0],
			ImGuiColorEditFlags_Float
		);

		ImGui::SliderFloat(
		    "Fog Density",
		    &m_Fog.Density,
		    0.0001f,
		    0.1f,
		    "%.4f",
		    ImGuiSliderFlags_Logarithmic
		);

		// =========================
		// CAMERA (GLOBAL)
		// =========================
		ImGui::Separator();
		ImGui::Text("Camera Controller");
		
		auto& cam = m_Controller.GetCamera();
		
		// NOTE: depends on your Camera API
		float fov = cam.GetFOV();
		float nearClip = cam.GetNearClip();
		float farClip = cam.GetFarClip();
		
		if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
		    cam.SetFOV(fov);
		
		if (ImGui::SliderFloat("Near Clip", &nearClip, 0.01f, 10.0f))
		    cam.SetNearClip(nearClip);
		
		if (ImGui::SliderFloat("Far Clip", &farClip, 10.0f, 10000.0f))
		    cam.SetFarClip(farClip);
		
		// =========================
		// FOLLOW CAMERA
		// =========================
		if (m_DebugFollow)
		{
		    ImGui::Separator();
		    ImGui::Text("Follow Camera");
		
		    ImGui::DragFloat3("Offset", &m_DebugFollow->Offset[0], 0.01f);
		
		    ImGui::DragFloat3("Rotation Offset", &m_DebugFollow->RotationOffset[0], 0.01f);
		}
		
		// =========================
		// ENGINE INFO
		// =========================

		ImGui::Separator();

		ImGui::Text("Press ESC to return to game");

		ImGui::End();
	}
}

void SandboxLayer::OnEvent(Event& e) { // I DONT THINK THIS SHOULD BE HERE:
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
        m_Controller.OnResize((float)e.GetWidth(), (float)e.GetHeight());
        return false;
    });
}

}
