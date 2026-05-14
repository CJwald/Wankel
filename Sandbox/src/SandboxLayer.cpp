#include "SandboxLayer.h"
#include "cube.h"
#include "plate.h"
#include "triangle.h"
#include "PLYLoader.h"
#include "Debug/DebugOverlay.h"
#include "MeshLoader.h"

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

SandboxLayer::SandboxLayer() : Layer("Cube"), m_Controller(1280.0f / 720.0f) {

	// Mesh setup
	m_ShipMesh = MeshLoader::Load("Assets/Mesh/SHIP04.ply");
	m_GunMesh = MeshLoader::Load("Assets/Mesh/AK74_IRONS.ply");
	m_BoxMesh = MeshLoader::Load("Assets/Mesh/Karachi.ply");
	
	m_CubeMesh = std::make_unique<Mesh>(Geometry::CubeVertices, sizeof(Geometry::CubeVertices), Geometry::CubeIndices, sizeof(Geometry::CubeIndices) / sizeof(uint32_t));

	// Shader
	m_Shader = std::make_unique<Shader>(
		"shaders/cube.vert", 
		"shaders/cube.frag"
	);

    // PLAYER ENTITY
    auto player = m_Scene.CreateEntity();
	auto& pt = player.AddComponent<TransformComponent>();
    pt.LocalPosition = {0,1,0};

    player.AddComponent<MeshComponent>().MeshPtr = m_ShipMesh.get();
    player.AddComponent<PlayerControllerComponent>();
    auto& anim = player.AddComponent<MeshAnimationComponent>();
    anim.PositionSpring = SecondOrderDynamics(2.0f, 0.8f, 2.0f, glm::vec3(0.0f));
    anim.RotationSpring = SecondOrderDynamics(2.0f, 0.8f, 2.0f, glm::vec3(0.0f));

    // PLAYER Gun ENTITY
	auto gun = m_Scene.CreateEntity();
	auto& gt = gun.AddComponent<TransformComponent>();
    gt.LocalPosition = {0.0f ,0.1f ,-0.125f};
	gun.AddComponent<MeshComponent>().MeshPtr = m_GunMesh.get();
    auto& gunAnim = gun.AddComponent<MeshAnimationComponent>();
    gunAnim.PositionSpring = SecondOrderDynamics(2.0f, 0.8f, 2.0f, glm::vec3(0.0f));
    gunAnim.RotationSpring = SecondOrderDynamics(2.0f, 0.8f, 2.0f, glm::vec3(0.0f));
	gun.AddComponent<ParentComponent>().Parent = player;

    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
    camEntity.AddComponent<TransformComponent>();
    auto& follow = camEntity.AddComponent<FollowCameraComponent>();

    follow.Target = player;
    //follow.Offset = {0.0f, 0.12f, 0.0f}; // I think i want to get this close to 0.0 and define mesh around that
    follow.Offset = {0.0f, 0.2f, 0.4f}; // I think i want to get this close to 0.0 and define mesh around that
	float roll = 0.0f; float pitch = 0.0f; float yaw = 0.0f; 
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


    // RANDOM CUBES 
	int numCubes = 20;
	float spawnRange = 25.f;
    for (int i = 0; i < numCubes; i++) {
        auto e = m_Scene.CreateEntity();

        auto& t = e.AddComponent<TransformComponent>();
		float X = RandomFloat() * spawnRange;
		float Y = RandomFloat() * spawnRange;
		float Z = RandomFloat() * spawnRange;
        t.LocalPosition = {X, Y, Z};
        t.LocalOrientation = 
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

    // WORLD 
	auto b = m_Scene.CreateEntity();
    auto& tb = b.AddComponent<TransformComponent>();
    tb.LocalPosition = {0.0f, 0.0f, 0.0f};
    tb.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	b.AddComponent<MeshComponent>().MeshPtr = m_BoxMesh.get();
    auto& rb_box = b.AddComponent<RigidbodyComponent>();
    rb_box.IsStatic = true;
	
    //// WORLD BOX 2
	//auto b2 = m_Scene.CreateEntity();
    //auto& tb2 = b2.AddComponent<TransformComponent>();
    //tb2.LocalPosition = {0.0f, 0.0f, 250.0f};
    //tb2.LocalOrientation = 
    //	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    //	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,1,0)) *
    //	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	//b2.AddComponent<MeshComponent>().MeshPtr = m_BoxMesh.get();
    //auto& rb_box2 = b2.AddComponent<RigidbodyComponent>();
    //rb_box2.IsStatic = true;


	// DEFAULT FOG
	//m_Fog.Color = {0.055f, 0.05f, 0.06f};
	m_Fog.Color = {0.32f, 0.3f, 0.38f};
	m_Fog.Density = 0.01f;

	// Lock mouse initially
	auto& window = Application::Get().GetWindow();
	GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window.GetNativeWindow());
	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}


void SandboxLayer::OnUpdate() {
	float time = Time::GetTime();
	float dt = time - m_LastFrame;
	m_LastFrame = time;

	DebugOverlay::PushFrameTime(dt);

	// ADD ESC to switch to ImGui
	static bool escPressedLastFrame = false;
	bool escPressed = Input::IsKeyPressed(Key::Escape);
	if (escPressed && !escPressedLastFrame) {
		m_GameFocused = !m_GameFocused;
		auto& window = Application::Get().GetWindow();
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window.GetNativeWindow());
		if (m_GameFocused) {
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else {
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	escPressedLastFrame = escPressed;

	m_PlayerInputSystem.Update(m_Scene, dt, m_GameFocused);

    m_Scene.OnUpdate(dt, m_Controller.GetCamera());
	
	auto playerView = m_Scene.Registry().view<TransformComponent, PlayerControllerComponent>();

	for (auto entity : playerView) {
    	auto& playerTransform = playerView.get<TransformComponent>(entity);

		// Wrap player if they chunk border limit
		glm::vec3 pos = playerTransform.LocalPosition;
		pos = glm::mod(pos + m_ChunkSize*0.5f, m_ChunkSize) - m_ChunkSize*0.5f;
		playerTransform.LocalPosition = pos;
	}


	Renderer::SetFog(m_Fog);

	auto& cam = m_Controller.GetCamera();
	glm::vec3 camPos = cam.GetPosition();
	glm::vec3 camForward = cam.GetForward();
    
	Renderer::BeginScene(cam);

	auto view = m_Scene.Registry().view<TransformComponent, MeshComponent>();

	// X Y Z loop for tiling world
	for (int ix = -m_RepeatN; ix <= m_RepeatN; ix++) {
	    for (int iy = -m_RepeatN; iy <= m_RepeatN; iy++) {
	        for (int iz = -m_RepeatN; iz <= m_RepeatN; iz++) {
	            glm::vec3 worldOffset = glm::vec3(ix, iy, iz) * m_ChunkSize;
			
				if (!(ix == 0 && iy == 0 && iz == 0)) { 	
					// check if chunk behind camera
					glm::vec3 chunkCenter = worldOffset;
					glm::vec3 toChunk = chunkCenter - camPos;
					float chunklen = glm::length(toChunk);
					if (chunklen > 0.001f) { 
						float d = glm::dot( toChunk / chunklen, camForward );
						
						// Skip chunks behind camera
						if (d < -0.2f)
						    continue;
					}
				}

				// Render
				for (auto entity : view) {

					auto& transform = view.get<TransformComponent>(entity);
    			    auto& mesh = view.get<MeshComponent>(entity);

					//glm::mat4 model = transform.GetLocalTransform();
					//glm::mat4 model = transform.WorldTransform;
					glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.WorldTransform;

					// PROCEDURAL MESH animATION
					if (m_Scene.Registry().all_of<MeshAnimationComponent>(entity)) {
					    auto& anim = m_Scene.Registry().get<MeshAnimationComponent>(entity);

						glm::vec3 rot = glm::radians(anim.RotationOffset);

						glm::quat pitch = glm::angleAxis(rot.x, glm::vec3(1,0,0));
						glm::quat yaw = glm::angleAxis(rot.y, glm::vec3(0,1,0));
						glm::quat roll = glm::angleAxis(rot.z, glm::vec3(0,0,1));
						glm::quat animRotation = glm::normalize(roll * yaw * pitch);

					    glm::mat4 animTransform = glm::translate(glm::mat4(1.0f), anim.PositionOffset) * glm::toMat4(animRotation);

					    // Apply animation in LOCAL SPACE
					    model = model * animTransform;
					}

    			    Renderer::Submit(model, *mesh.MeshPtr, m_Shader.get());
				}

			} // Z
		} // Y
	} // X

	Renderer::EndScene();
}


void SandboxLayer::OnImGuiRender() {
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->GetGlyphRangesDefault();
	if (!m_GameFocused) {
		ImGui::Begin("Renderer Debug");

		DebugOverlay::DrawFPSPanel();
		
		// FOG
		if (ImGui::CollapsingHeader("Fog")) { 
			if (ImGui::CollapsingHeader("Color Picker")) { 
				ImGui::SameLine();
				ImGui::ColorPicker3("##FogColor", 
					&m_Fog.Color[0], 
					ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float
				);
			}

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

			ImGui::Separator();
			ImGui::Checkbox(
			    "Noise Fog",
			    &m_Fog.NoiseEnabled
			);
			
			ImGui::SliderFloat(
			    "Noise Scale",
			    &m_Fog.NoiseScale,
			    0.0001f,
			    0.1f,
			    "%.5f",
			    ImGuiSliderFlags_Logarithmic
			);
			
			ImGui::SliderFloat(
			    "Noise Strength",
			    &m_Fog.NoiseStrength,
			    0.0f,
			    4.0f
			);
			
			ImGui::SliderInt(
			    "Noise Octaves",
			    &m_Fog.NoiseOctaves,
			    1,
			    8
			);

			ImGui::Separator();
			ImGui::Text("Fog Wind");
			ImGui::DragFloat3(
			    "Wind Direction",
			    &m_Fog.WindDir[0],
			    0.01f
			);
			
			ImGui::SliderFloat(
			    "Wind Speed",
			    &m_Fog.WindSpeed,
			    0.0f,
			    5.0f
			);
			
			// normalize direction to avoid weird scaling
			if (glm::length(m_Fog.WindDir) > 0.0001f)
			    m_Fog.WindDir = glm::normalize(m_Fog.WindDir);
		}

		// CAMERA (GLOBAL)
		if (ImGui::CollapsingHeader("Camera")) { 
			auto& cam = m_Controller.GetCamera();
			float fov = cam.GetFOV();
			float nearClip = cam.GetNearClip();
			float farClip = cam.GetFarClip();
			
			ImGui::Text("Camera Settings");
			if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
			    cam.SetFOV(fov);
			if (ImGui::SliderFloat("Near Clip", &nearClip, 0.001f, 5.0f))
			    cam.SetNearClip(nearClip);
			if (ImGui::SliderFloat("Far Clip", &farClip, 10.0f, 10000.0f))
			    cam.SetFarClip(farClip);
			
			glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(m_DebugFollow->RotationOffset));
			if (m_DebugFollow) {
			    ImGui::Separator();
			    ImGui::Text("Follow Camera");
			    ImGui::DragFloat3("Offset", &m_DebugFollow->Offset[0], 0.01f);
			    ImGui::DragFloat3("Rotation Offset [pyr]", &eulerDeg[0], 0.1f);
				glm::vec3 eulerRad = glm::radians(eulerDeg);
    			m_DebugFollow->RotationOffset = glm::quat(eulerRad);
			}
		}

		// PLAYER ANIMATION
		if (ImGui::CollapsingHeader("animation")) { 
			auto animView = m_Scene.Registry().view<MeshAnimationComponent>();
			
			for (auto entity : animView) {
			    auto& anim = animView.get<MeshAnimationComponent>(entity);
			
			    ImGui::Separator();
			
			    // POSITION AMPLITUDE
			    ImGui::Text("Position");
				glm::vec3 posAmpUI = anim.PositionAmplitude * 1000.0f;
			    if (ImGui::DragFloat3(
			        "Amplitude##Position",
			        &posAmpUI[0],
			        1.0f,
			        0.0f,
			        1000.0f
			    	)) {
					anim.PositionAmplitude = posAmpUI / 1000.0f;
				}
			
			    // ROTATION AMPLITUDE
			    ImGui::Text("Rotation");
			    ImGui::DragFloat3(
			        "Amplitude##Rotation",
			        &anim.RotationAmplitude[0],
			        0.1f,
			        0.0f,
			        50.0f
			    );
			
			    ImGui::Separator();
			    ImGui::Text("Position Tuning");
			    ImGui::SliderFloat(
			        "w - Frequency##Pos",
			        &anim.PositionFrequency,
			        0.1f,
			        10.0f
			    );
			
			    ImGui::SliderFloat(
			        "z - Damping##Pos",
			        &anim.PositionDamping,
			        0.0f,
			        3.0f
			    );
			
			    ImGui::SliderFloat(
			        "r - Response##Pos",
			        &anim.PositionResponse,
			        -3.0f,
			        3.0f
			    );
			
			    ImGui::Separator();
			    ImGui::Text("Rotation Tuning");
			    ImGui::SliderFloat(
			        "w - Frequency##Rot",
			        &anim.RotationFrequency,
			        0.1f,
			        10.0f
			    );
			
			    ImGui::SliderFloat(
			        "z - Damping##Rot",
			        &anim.RotationDamping,
			        0.0f,
			        3.0f
			    ); //ζ
			
			    ImGui::SliderFloat(
			        "r - Response##Rot",
			        &anim.RotationResponse,
			        -3.0f,
			        3.0f
			    );
			
			    break;
			}
		}

		// WORLD DEBUG
		if (ImGui::CollapsingHeader("World Tiling"))
		{
		    ImGui::SliderFloat(
		        "Chunk Size",
		        &m_ChunkSize,
		        1.0f,
		        500.0f,
		        "%.1f"
		    );
		
		    ImGui::SliderInt(
		        "Repeat N",
		        &m_RepeatN,
		        0,
		        10
		    );
		
		    ImGui::Text("Grid: %d x %d x %d",
		        2 * m_RepeatN + 1,
		        2 * m_RepeatN + 1,
		        2 * m_RepeatN + 1
		    );
		}

		// ENGINE INFO
		ImGui::Separator();
		ImGui::Text("Press ESC to return to game");
		ImGui::End();
	}
}


void SandboxLayer::OnEvent(Event& e) { // TODO: I DONT KNOW IF THIS SHOULD BE HERE, MAYBE ENGINE SHOULD HANDLE?
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
        m_Controller.OnResize((float)e.GetWidth(), (float)e.GetHeight());
        return false;
    });
}


}
