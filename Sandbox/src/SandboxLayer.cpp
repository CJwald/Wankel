#include "SandboxLayer.h"
#include "cube.h"
#include "plate.h"
#include "triangle.h"
#include "PLYLoader.h"
#include "Debug/DebugOverlay.h"
#include "Debug/SecondOrderPreview.h"
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
#include <Wankel/Renderer/DebugDraw.h>

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

static const char* MotionAxisName(MotionAxis axis) {
    switch (axis) {
        case MotionAxis::PosX: return "PosX";
        case MotionAxis::PosY: return "PosY";
        case MotionAxis::PosZ: return "PosZ";

        case MotionAxis::RotX: return "RotX";
        case MotionAxis::RotY: return "RotY";
        case MotionAxis::RotZ: return "RotZ";
    }
    return "Unknown";
}

static const char* MotionAxisLabels[] = {
    "PosX",
    "PosY",
    "PosZ",
    "RotX",
    "RotY",
    "RotZ"
};

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
	// Forward velocity -> pitch
	{
	    auto& link = anim.Links[ (int)MotionAxis::PosZ ][ (int)MotionAxis::RotX ];
	
	    link.Enabled = true;
	    link.Magnitude = -0.2f;
	    link.Frequency = 1.8f;
	    link.Damping = 0.4f;
	    link.Response = 2.0f;
	    link.Clamp = 8.0f;
	}
	
	// Strafing -> roll
	{
	    auto& link = anim.Links[ (int)MotionAxis::PosX ][ (int)MotionAxis::RotZ ];
	
	    link.Enabled = true;
	    link.Magnitude = -0.4f;
	    link.Frequency = 2.0f;
	    link.Damping = 0.5f;
	    link.Response = 2.0f;
	    link.Clamp = 10.0f;
	}
	
	// Vertical velocity -> vertical bob
	{
	    auto& link = anim.Links[ (int)MotionAxis::PosY ][ (int)MotionAxis::PosY ];
	
	    link.Enabled = true;
	    link.Magnitude = -0.002f;
	    link.Frequency = 2.5f;
	    link.Damping = 0.7f;
	    link.Response = 1.5f;
	    link.Clamp = 0.05f;
	}
	
	// YAW -> Yaw Rot
	auto& YawRoll = anim.Links[ (int)MotionAxis::RotY ][ (int)MotionAxis::RotZ ];
	YawRoll.Enabled = true;
	YawRoll.Magnitude = -0.002f;
	YawRoll.Frequency = 2.5f;
	YawRoll.Damping = 0.7f;
	YawRoll.Response = -1.5f;
	YawRoll.Clamp = 9999.99f; // no clamp
	

    // PLAYER Gun ENTITY
	auto gun = m_Scene.CreateEntity();
	auto& gt = gun.AddComponent<TransformComponent>();
    gt.LocalPosition = {0.05f ,0.08f ,-0.125f};
	gun.AddComponent<MeshComponent>().MeshPtr = m_GunMesh.get();
	gun.AddComponent<ParentComponent>().Parent = player;

    // PLAYER Gun ENTITY2
	auto gun2 = m_Scene.CreateEntity();
	auto& gt2 = gun2.AddComponent<TransformComponent>();
    gt2.LocalPosition = {-0.05f ,0.08f ,-0.125f};
	gun2.AddComponent<MeshComponent>().MeshPtr = m_GunMesh.get();
	gun2.AddComponent<ParentComponent>().Parent = player;

    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
    camEntity.AddComponent<TransformComponent>();
    auto& follow = camEntity.AddComponent<FollowCameraComponent>();

    follow.Target = player;
    //follow.Offset = {0.0f, 0.15f, 0.0f};
    follow.Offset = {0.0f, 0.15f, 0.4f};
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
	

	// DEFAULT FOG
	m_Fog.Color = {0.32f, 0.3f, 0.38f};
	m_Fog.Density = 0.008f;

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
	//  This needs to be factored out of this lol
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
						if (d < -0.1f)
						    continue;
					}
				}

				// Render
				for (auto entity : view) {
					auto& transform = view.get<TransformComponent>(entity);
    			    auto& mesh = view.get<MeshComponent>(entity);
					glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform * mesh.GetLocalTransform();

					// DEBUG AXES
					if (Renderer::DebugEnabled) {
					    glm::vec3 origin = glm::vec3(model[3]);
					    float axisLength = 0.125f;
					
					    glm::vec3 right = glm::normalize(glm::vec3(model[0])) * axisLength;
					    glm::vec3 up = glm::normalize(glm::vec3(model[1])) * axisLength;
					    glm::vec3 forward = glm::normalize(glm::vec3(model[2])) * axisLength;
					
					    std::vector<DebugLine> lines = {
					        { origin, origin + right,   {0.8, 0.3, 0.0} },// X axis (Right)
					        { origin, origin + up,      {0.6, 1.0, 0.0} },// Y axis (Up)
					        { origin, origin + forward, {0.4, 0.0, 0.9} } // Z axis (Backward)
					    };
					
					    // Parent link
					    if (m_Scene.Registry().all_of<ParentComponent>(entity)) {
					        auto parent = m_Scene.Registry().get<ParentComponent>(entity).Parent;
					        if (parent) {
								auto& childTransform = m_Scene.Registry().get<TransformComponent>(entity);
					            auto& parentTransform = parent.GetComponent<TransformComponent>();
								glm::vec3 childPos = glm::vec3(childTransform.WorldTransform[3]);
					            glm::vec3 parentPos = glm::vec3(parentTransform.WorldTransform[3]);
					            lines.push_back({childPos, parentPos, {1,1,1}});
					        }
					    }
					    Renderer::SubmitDebugLines(lines);
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
			ImGui::ColorEdit3("Fog Color", &m_Fog.Color[0], ImGuiColorEditFlags_Float);
			ImGui::SliderFloat("Fog Density", &m_Fog.Density, 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

			ImGui::Separator();
			ImGui::Checkbox("Noise Fog", &m_Fog.NoiseEnabled);
			ImGui::SliderFloat("Noise Scale", &m_Fog.NoiseScale, 0.0001f, 0.1f, "%.5f", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Noise Strength", &m_Fog.NoiseStrength, 0.0f, 4.0f);
			ImGui::SliderInt("Noise Octaves", &m_Fog.NoiseOctaves, 1, 8);

			ImGui::Separator();
			ImGui::Text("Fog Wind");
			ImGui::DragFloat3("Wind Direction", &m_Fog.WindDir[0], 0.01f);
			
			ImGui::SliderFloat("Wind Speed", &m_Fog.WindSpeed, 0.0f, 5.0f);
			
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
			if (ImGui::SliderFloat("FOV (Vertical)", &fov, 30.0f, 120.0f))
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
		if (ImGui::CollapsingHeader("Animation")) { 
			auto animView = m_Scene.Registry().view<MeshAnimationComponent>();
			static int selectedOutputAxis[MeshAnimationComponent::AxisCount] = {};
			for (auto entity : animView) {
			    auto& anim = animView.get<MeshAnimationComponent>(entity);
			    for (int input = 0; input < MeshAnimationComponent::AxisCount; input++) {
			        MotionAxis inAxis = (MotionAxis)input;
			        if (!ImGui::TreeNode(MotionAxisName(inAxis)))
			            continue;
			        bool anyShown = false;
			
			        // EXISTING MAPPINGS
			        for (int output = 0; output < MeshAnimationComponent::AxisCount; output++) {
			            auto& link = anim.Links[input][output];
			
			            if (!link.Enabled)
			                continue;
			
			            anyShown = true;
			            MotionAxis outAxis = (MotionAxis)output;
			            std::string label = std::string(MotionAxisName(inAxis)) + " -> " + MotionAxisName(outAxis);
			            bool removeMapping = false;
			            if (ImGui::TreeNode(label.c_str())) {
			                ImGui::Checkbox(("Enabled##" + label).c_str(), &link.Enabled);
			                ImGui::DragFloat(("Magnitude##" + label).c_str(), &link.Magnitude, 0.01f);
			                ImGui::DragFloat(("Frequency##" + label).c_str(), &link.Frequency, 0.01f, 0.01f, 20.0f);
			                ImGui::DragFloat(("Damping##" + label).c_str(), &link.Damping, 0.01f, 0.0f, 10.0f);
			                ImGui::DragFloat(("Response##" + label).c_str(), &link.Response, 0.01f, -10.0f, 10.0f);
			                ImGui::DragFloat(("Clamp##" + label).c_str(), &link.Clamp, 0.01f, 0.0f, 1000.0f);
			                ImGui::Text("Output: %.3f", link.Output);


							// Position Step Response
							auto pvalues = Wankel::SecondOrderPreview::GetStepResponse(
							    link.Frequency,
							    link.Damping,
							    link.Response
							);
							ImGui::PlotLines("Step Response##Pos", pvalues.data(), (int)pvalues.size(), 0, nullptr, -0.5f, 2.0f, ImVec2(0, 100));




			                ImGui::Separator();
			                if (ImGui::Button(("Remove##" + label).c_str())) {
			                    removeMapping = true;
			                }
			
			                ImGui::TreePop();
			            }
			
			            // REMOVE AFTER UI
			            if (removeMapping) {
			                link = {};
			            }
			        }
			
			        if (!anyShown) {
			            ImGui::TextDisabled("No active mappings");
			        }
			
			        ImGui::Separator();
			
			        // ADD NEW MAPPING
			        ImGui::SetNextItemWidth(120.0f);
			        std::string buttonId = "+ Add Mapping##" + std::to_string(input);
			
			        if (ImGui::Button(buttonId.c_str())) {
			            int output = selectedOutputAxis[input];
			            auto& link = anim.Links[input][output];
			
			            // Only initialize if newly created
			            if (!link.Enabled) {
			
			                link.Enabled = true;
			
			                link.Magnitude = 1.0f;
			                link.Frequency = 2.0f;
			                link.Damping = 0.5f;
			                link.Response = 1.0f;
			                link.Clamp = 10.0f;
			            }
			        }
			        ImGui::SameLine();
			        std::string comboId = "##AddMappingCombo" + std::to_string(input);
			        ImGui::Combo(comboId.c_str(), &selectedOutputAxis[input], MotionAxisLabels, MeshAnimationComponent::AxisCount);
			
			        ImGui::TreePop();
			    }
			
			    break;
			}

		}

		// WORLD DEBUG
		if (ImGui::CollapsingHeader("World Tiling"))
		{
		    ImGui::SliderFloat("Chunk Size", &m_ChunkSize, 1.0f, 500.0f, "%.1f");
		    ImGui::SliderInt("Repeat N", &m_RepeatN, 0, 10);
		    ImGui::Text("Grid: %d x %d x %d", 2 * m_RepeatN + 1, 2 * m_RepeatN + 1, 2 * m_RepeatN + 1);
		}

		ImGui::Checkbox("Debug Draw", &Renderer::DebugEnabled);


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
