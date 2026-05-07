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

// To-be removed eventually
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

float RandomFloat()
{
    static std::random_device rd;  // seed
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    return dist(gen);
}

namespace Wankel {

SandboxLayer::SandboxLayer()
	: Layer("Cube"), m_Controller(1280.0f / 720.0f)
{

	auto meshData = PLYLoader::Load("Assets/Mesh/ShipGrey.ply");
	std::cout << "Verts: " << meshData.Vertices.size() << std::endl;
	std::cout << "Indices: " << meshData.Indices.size() << std::endl;
	m_ShipMesh = std::make_unique<Mesh>(
	    meshData.Vertices.data(),
	    meshData.Vertices.size() * sizeof(float),
	    meshData.Indices.data(),
	    meshData.Indices.size() * sizeof(uint32_t)
	);

	// Mesh setup
	m_CubeMesh = std::make_unique<Mesh>(
	    Geometry::CubeVertices,
	    sizeof(Geometry::CubeVertices),
	    Geometry::CubeIndices,
	    sizeof(Geometry::CubeIndices)
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

    //player.AddComponent<MeshComponent>().MeshPtr = m_TriangleMesh.get();
    player.AddComponent<MeshComponent>().MeshPtr = m_ShipMesh.get();
    player.AddComponent<PlayerControllerComponent>();

    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
    camEntity.AddComponent<TransformComponent>();
    auto& follow = camEntity.AddComponent<FollowCameraComponent>();

    follow.Target = player;
    //follow.Offset = {-0.125f, 0.5f, 1.5f};
    //follow.Offset = {-0.05f, 0.25f, 0.5f};
    follow.Offset = {-0.0f, 0.25f, 0.5f};
	float roll = 0.0f; float pitch = -4.0f; float yaw = 0.0f; //-1.0f; 
	follow.RotationOffset =
    	glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(yaw), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(roll), glm::vec3(0,0,1));

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
}


void SandboxLayer::OnUpdate() {
	float time = Time::GetTime();
	float dt = time - m_LastFrame;
	m_LastFrame = time;

	// =========================
    // INPUT → CONTROLLER (GAME LOGIC)
    // =========================
 	auto controllerView = m_Scene.Registry().view<PlayerControllerComponent>();

    for (auto entity : controllerView) {
        auto& controller = controllerView.get<PlayerControllerComponent>(entity);

        glm::vec3 input(0.0f);
		float rollInput(0.0f);
		
        // BOOST
		controller.BoostMultiplier = 2.0f;

        if (Input::IsKeyPressed(Key::W)) input.z += 1.0f;
        if (Input::IsKeyPressed(Key::S)) input.z -= 1.0f;
        if (Input::IsKeyPressed(Key::D)) input.x += 1.0f;
        if (Input::IsKeyPressed(Key::A)) input.x -= 1.0f;
        if (Input::IsKeyPressed(Key::Space)) input.y += 1.0f;
        if (Input::IsKeyPressed(Key::LeftControl)) input.y -= 1.0f;
        if (Input::IsKeyPressed(Key::Q)) rollInput = -1.0f;
        if (Input::IsKeyPressed(Key::E)) rollInput = 1.0f;
        controller.Boost = Input::IsKeyPressed(Key::LeftShift);

		// =========================
	    // CONTROLLER (Player 0)
	    // =========================
	    int pad = 0;

	    float lx = ControllerInput::GetAxis(pad, 0); // LEFT_X
	    float ly = ControllerInput::GetAxis(pad, 1); // LEFT_Y
	    float rx = ControllerInput::GetAxis(pad, 2); // RIGHT_X
	    float ry = ControllerInput::GetAxis(pad, 3); // RIGHT_Y

	    // Movement (left stick)
	    input.x += lx;
	    input.z += -ly; // invert Y (forward)

	    // Vertical movement via triggers
	    float lt = ControllerInput::GetAxis(pad, 4);
	    float rt = ControllerInput::GetAxis(pad, 5);
	    input.y += rt - lt;
	    
	    // Look (right stick)
    	controller.LookDeltaX = rx * 10.0f; // scale to feel like mouse
    	controller.LookDeltaY = ry * 10.0f;

    	// Roll (optional: shoulder buttons)
    	if (ControllerInput::IsButtonPressed(pad, 4)) rollInput = -1.0f; // LB
    	if (ControllerInput::IsButtonPressed(pad, 5)) rollInput = 1.0f;  // RB

    	// BOOST (R3 click)
	    controller.Boost = ControllerInput::IsButtonPressed(pad, 9);
    
    
    
    
    
    
        controller.MoveInput = input;
        controller.RollInput = rollInput;
        if (rx == 0.0f && ry == 0.0f) {
			controller.LookDeltaX = Input::GetMouseDeltaX();
			controller.LookDeltaY = Input::GetMouseDeltaY();
		}

		WK_CORE_INFO("INPUT: [{0:.3f}3 {1:.3f}] | [{2:.3f}, {3:.3f}]", lx, ly, rx, ry);

    }

    m_Scene.OnUpdate(dt, m_Controller.GetCamera());

	Renderer::Clear(0.12f, 0.1f, 0.2f, 1.0f);

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

void SandboxLayer::OnEvent(Event& e) { // I DONT THINK THIS SHOULD BE HERE:
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
        m_Controller.OnResize((float)e.GetWidth(), (float)e.GetHeight());
        return false;
    });
}

}
