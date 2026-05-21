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
        case MotionAxis::X: return "X";
        case MotionAxis::Y: return "Y";
        case MotionAxis::Z: return "Z";

        case MotionAxis::Pitch: return "Pitch";
        case MotionAxis::Yaw: return "Yaw";
        case MotionAxis::Roll: return "Roll";
    }
    return "Unknown";
}

static const char* MotionAxisLabels[] = {
    "X",
    "Y",
    "Z",
    "Pitch",
    "Yaw",
    "Roll"
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
	player.AddComponent<TagComponent>().Name = "Player";
	auto& pt = player.AddComponent<TransformComponent>();
    pt.LocalPosition = {0,1,0};

    player.AddComponent<MeshComponent>().MeshPtr = m_ShipMesh.get();
    player.AddComponent<PlayerControllerComponent>();
    auto& anim = player.AddComponent<MeshAnimationComponent>();
	//anim.PositionOffset = {10.0f, 0.0f, 0.0f};// This doesnt seem to work
	//anim.RotationOffset = {0.0f, 0.0f, 0.0f};// This doesnt seem to work
	// Forward velocity -> pitch
	auto& ForwardPitch = anim.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	ForwardPitch.Enabled = true;
	ForwardPitch.Magnitude = -0.2f;
	ForwardPitch.Frequency = 1.8f;
	ForwardPitch.Damping = 0.4f;
	ForwardPitch.Response = 2.0f;
	ForwardPitch.Clamp = 8.0f;
	
	// Strafing -> roll
	auto& StrafeRoll = anim.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	StrafeRoll.Enabled = true;
	StrafeRoll.Magnitude = -0.4f;
	StrafeRoll.Frequency = 2.0f;
	StrafeRoll.Damping = 0.5f;
	StrafeRoll.Response = 2.0f;
	StrafeRoll.Clamp = 10.0f;
	
	// Vertical velocity -> vertical bob
	auto& VertBob = anim.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Y ];
	VertBob.Enabled = true;
	VertBob.Magnitude = -0.002f;
	VertBob.Frequency = 2.5f;
	VertBob.Damping = 0.7f;
	VertBob.Response = 1.5f;
	VertBob.Clamp = 0.05f;
	
	// YAW -> Yaw Rot
	auto& YawRoll = anim.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Roll ];
	YawRoll.Enabled = true;
	YawRoll.Magnitude = -0.2f;
	YawRoll.Frequency = 2.5f;
	YawRoll.Damping = 0.7f;
	YawRoll.Response = -1.5f;
	YawRoll.Clamp = 9999.99f; // no clamp
	

    // PLAYER Gun ENTITY
	auto gun1 = m_Scene.CreateEntity();
	gun1.AddComponent<TagComponent>().Name = "Gun1";
	auto& gt1 = gun1.AddComponent<TransformComponent>();
    gt1.LocalPosition = {0.05f ,0.08f ,-0.125f};
	gun1.AddComponent<MeshComponent>().MeshPtr = m_GunMesh.get();
	gun1.AddComponent<ParentComponent>().Parent = player;
    auto& gunAnim1 = gun1.AddComponent<MeshAnimationComponent>();
	// Forward velocity -> pitch
	auto& Gun1StrafeRoll = gunAnim1.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	Gun1StrafeRoll.Enabled = true;
	Gun1StrafeRoll.Magnitude = -1.2f;
	Gun1StrafeRoll.Frequency = 1.8f;
	Gun1StrafeRoll.Damping = 0.8f;
	Gun1StrafeRoll.Response = 2.0f;
	Gun1StrafeRoll.Clamp = 8.0f;
	auto& Gun1ForwardLag = gunAnim1.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	Gun1ForwardLag.Enabled = true;
	Gun1ForwardLag.Magnitude = 0.002f;
	Gun1ForwardLag.Frequency = 1.8f;
	Gun1ForwardLag.Damping = 0.8f;
	Gun1ForwardLag.Response = 2.0f;
	Gun1ForwardLag.Clamp = 8.0f;
	auto& Gun1YawYaw = gunAnim1.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Yaw ];
	Gun1YawYaw.Enabled = true;
	Gun1YawYaw.Magnitude = 1.2f;
	Gun1YawYaw.Frequency = 1.8f;
	Gun1YawYaw.Damping = 0.8f;
	Gun1YawYaw.Response = 2.0f;
	Gun1YawYaw.Clamp = 8.0f;
	auto& Gun1PitchPitch = gunAnim1.Links[ (int)MotionAxis::Pitch ][ (int)MotionAxis::Pitch ];
	Gun1PitchPitch.Enabled = true;
	Gun1PitchPitch.Magnitude = 1.2f;
	Gun1PitchPitch.Frequency = 1.8f;
	Gun1PitchPitch.Damping = 0.8f;
	Gun1PitchPitch.Response = 2.0f;
	Gun1PitchPitch.Clamp = 8.0f;

    // PLAYER Gun ENTITY2
	auto gun2 = m_Scene.CreateEntity();
	gun2.AddComponent<TagComponent>().Name = "Gun2";
	auto& gt2 = gun2.AddComponent<TransformComponent>();
    gt2.LocalPosition = {-0.05f ,0.08f ,-0.125f};
	gun2.AddComponent<MeshComponent>().MeshPtr = m_GunMesh.get();
	gun2.AddComponent<ParentComponent>().Parent = player;
    auto& gunAnim2 = gun2.AddComponent<MeshAnimationComponent>();
	// Forward velocity -> pitch
	auto& Gun2StrafeRoll = gunAnim2.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	Gun2StrafeRoll.Enabled = true;
	Gun2StrafeRoll.Magnitude = -1.2f;
	Gun2StrafeRoll.Frequency = 1.8f;
	Gun2StrafeRoll.Damping = 0.8f;
	Gun2StrafeRoll.Response = 2.0f;
	Gun2StrafeRoll.Clamp = 8.0f;
	auto& Gun2ForwardLag = gunAnim2.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	Gun2ForwardLag.Enabled = true;
	Gun2ForwardLag.Magnitude = 0.002f;
	Gun2ForwardLag.Frequency = 1.8f;
	Gun2ForwardLag.Damping = 0.8f;
	Gun2ForwardLag.Response = 2.0f;
	Gun2ForwardLag.Clamp = 8.0f;
	auto& Gun2YawYaw = gunAnim2.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Yaw ];
	Gun2YawYaw.Enabled = true;
	Gun2YawYaw.Magnitude = 1.2f;
	Gun2YawYaw.Frequency = 1.8f;
	Gun2YawYaw.Damping = 0.8f;
	Gun2YawYaw.Response = 2.0f;
	Gun2YawYaw.Clamp = 8.0f;
	auto& Gun2PitchPitch = gunAnim2.Links[ (int)MotionAxis::Pitch ][ (int)MotionAxis::Pitch ];
	Gun2PitchPitch.Enabled = true;
	Gun2PitchPitch.Magnitude = 1.2f;
	Gun2PitchPitch.Frequency = 1.8f;
	Gun2PitchPitch.Damping = 0.8f;
	Gun2PitchPitch.Response = 2.0f;
	Gun2PitchPitch.Clamp = 8.0f;

    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
	camEntity.AddComponent<TagComponent>().Name = "Player Camera";
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
		e.AddComponent<TagComponent>().Name = "Cube";

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
	b.AddComponent<TagComponent>().Name = "World";
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

		if (ImGui::CollapsingHeader("Entity")) {
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);
			ImGui::Indent();

			auto& registry = m_Scene.Registry();

			// BUILD ENTITY LIST
			std::vector<entt::entity> entities;
			auto& storage = registry.storage<entt::entity>();

			for (auto entity : storage)
				entities.push_back(entity);

			std::reverse(entities.begin(), entities.end());

			// default selection
			if (!entities.empty()) {
				bool valid = m_SelectedAnimEntity != entt::null && registry.valid(m_SelectedAnimEntity);

				if (!valid)
				    m_SelectedAnimEntity = entities[0];
			}

			// ENTITY DROPDOWN
			std::vector<std::string> storageNames;
			std::vector<const char*> labels;
			for (auto entity : entities) {
				std::string name = "Unknown";
				if (registry.all_of<TagComponent>(entity))
				    name = registry.get<TagComponent>(entity).Name;
				storageNames.push_back(name + " (" + std::to_string((uint32_t)entity) + ")");
			}

			for (auto& s : storageNames)
				labels.push_back(s.c_str());

			static int selectedIndex = 0;

			if (!labels.empty()) {
				if (ImGui::Combo("Selected Entity", &selectedIndex, labels.data(), (int)labels.size())) {
				    m_SelectedAnimEntity = entities[selectedIndex];
				}
			}

			ImGui::Separator();

			// ANIMATION SECTION (ALWAYS VISIBLE)
			if (ImGui::CollapsingHeader("Animation")) {
				bool validEntity = m_SelectedAnimEntity != entt::null && registry.valid(m_SelectedAnimEntity);
				if (!validEntity) {
				    ImGui::TextDisabled("No entity selected.");
				}
				else if (!registry.all_of<MeshAnimationComponent>(m_SelectedAnimEntity)) {
				    // NO COMPONENT → ADD BUTTON
				    if (ImGui::Button("Add MeshAnimationComponent")) {
				        registry.emplace<MeshAnimationComponent>(m_SelectedAnimEntity);
				    }
				}
				else {
				    // EDIT EXISTING COMPONENT
				    auto& anim = registry.get<MeshAnimationComponent>(m_SelectedAnimEntity);
				    static int selectedOutputAxis[MeshAnimationComponent::AxisCount] = {};
				    for (int input = 0; input < MeshAnimationComponent::AxisCount; input++) {
				        MotionAxis inAxis = (MotionAxis)input;

				        if (!ImGui::TreeNode(MotionAxisName(inAxis)))
				            continue;

				        bool anyShown = false;
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
				                ImGui::Separator();

				                if (ImGui::Button(("Remove##" + label).c_str()))
				                    removeMapping = true;

				                ImGui::TreePop();
				            }

				            if (removeMapping)
				                link = {};
				        }

				        if (!anyShown)
				            ImGui::TextDisabled("No active mappings");

				        ImGui::Separator();

				        // ADD NEW LINK
				        std::string buttonId = "+ Add Mapping##" + std::to_string(input);

				        if (ImGui::Button(buttonId.c_str())) {
				            int output = selectedOutputAxis[input];
				            auto& link = anim.Links[input][output];

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
				}
			}

			ImGui::Unindent();
			ImGui::PopStyleVar();
		}


		// WORLD DEBUG
		if (ImGui::CollapsingHeader("World Tiling")) {
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
