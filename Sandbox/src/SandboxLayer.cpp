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
#include <Wankel/Physics/Raycast/Ray.h>
#include <Wankel/Physics/Raycast/Raycast.h>
#include <Wankel/Physics/Raycast/RaycastHit.h>

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

Ray CreateCameraRay(const Camera& cam, glm::vec3 origin) {
    Ray ray;
    ray.Origin = origin; //cam.GetPosition() + glm::normalize(cam.GetForward()) * 1.6f;
    ray.Direction = glm::normalize(cam.GetForward());
    return ray;
}

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
	//m_ShipMesh = MeshLoader::Load("Assets/Mesh/SHIP04.ply");
	m_ShipMesh = MeshLoader::Load("Assets/Mesh/SHIP05.ply");
	m_GunMesh = MeshLoader::Load("Assets/Mesh/AK74_IRONS.ply");
	//m_BoxMesh = MeshLoader::Load("Assets/Mesh/Karachi.ply");
	//m_BoxMesh = MeshLoader::Load("Assets/Mesh/Flat200m.ply");
	m_BoxMesh = MeshLoader::Load("Assets/Mesh/Rolling200m.ply");
	//m_BoxMesh = MeshLoader::Load("Assets/Mesh/Rolling1000m.ply");
	m_PlayerHeadMesh = MeshLoader::Load("Assets/Mesh/PlayerHead01.ply");
	m_PlayerLegMesh = MeshLoader::Load("Assets/Mesh/PlayerLeg01.ply");
	m_EnemyBodyMesh = MeshLoader::Load("Assets/Mesh/StalkerBody01.ply");
	m_EnemyLegMesh = MeshLoader::Load("Assets/Mesh/StalkerLeg01.ply");
	m_CubeMesh = std::make_unique<Mesh>(Geometry::CubeVertices, Geometry::CubeIndices);

	// Shader
	m_Shader = std::make_unique<Shader>(
		"shaders/cube.vert", 
		"shaders/cube.frag"
	);

    // PLAYER ENTITY
    auto player = m_Scene.CreateEntity();
	player.AddComponent<Tag>().Name = "Player";
	auto& pt = player.AddComponent<Transform>();
	auto& kin = player.AddComponent<Kinematics>();
    pt.LocalPosition = {0,1,0};
    player.AddComponent<PlayerController>();

	// head 
	auto phead = m_Scene.CreateEntity();
	{
	phead.AddComponent<Tag>().Name = "Player Head";
	auto& tc = phead.AddComponent<Transform>();
	auto& kin = phead.AddComponent<Kinematics>();
    tc.LocalPosition = {0.0f,0.0f,0.0f};
	phead.AddComponent<Parent>().Parent = player;
    phead.AddComponent<MeshRenderer>().MeshPtr = m_PlayerHeadMesh.get();
    auto& pheadAnim = phead.AddComponent<MeshAnimation>();
	// Strafe -> x
	auto& pheadXX = pheadAnim.Links[ (int)MotionAxis::X ][ (int)MotionAxis::X ];
	pheadXX.Enabled = true;
	pheadXX.Magnitude = 0.5f *0.01f;
	pheadXX.Frequency = 1.6f;
	pheadXX.Damping = 0.6f;
	pheadXX.Response = 1.4f;
	pheadXX.Clamp = 45.0f;
	// Strafe -> roll
	auto& pheadXR = pheadAnim.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	pheadXR.Enabled = true;
	pheadXR.Magnitude = -0.5f;
	pheadXR.Frequency = 1.6f;
	pheadXR.Damping = 1.6f;
	pheadXR.Response = 1.4f;
	pheadXR.Clamp = 45.0f;
	// Forward -> Pitch
	auto& pheadZP = pheadAnim.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	pheadZP.Enabled = true;
	pheadZP.Magnitude = 0.2f;
	pheadZP.Frequency = 1.6f;
	pheadZP.Damping = 1.6f;
	pheadZP.Response = 1.4f;
	pheadZP.Clamp = 45.0f;
	// Yaw -> yaw
	auto& pheadYY = pheadAnim.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Yaw ];
	pheadYY.Enabled = true;
	pheadYY.Magnitude = 5.0f;
	pheadYY.Frequency = 0.6f;
	pheadYY.Damping = 1.6f;
	pheadYY.Response = 1.6f;
	pheadYY.Clamp = 45.0f;
	// Pitch -> pitch
	auto& pheadPP = pheadAnim.Links[ (int)MotionAxis::Pitch ][ (int)MotionAxis::Pitch ];
	pheadPP.Enabled = true;
	pheadPP.Magnitude = 5.0f;
	pheadPP.Frequency = 0.6f;
	pheadPP.Damping = 1.6f;
	pheadPP.Response = 1.6f;
	pheadPP.Clamp = 45.0f;
	}

	// Leg1 
	{
	auto pLeg1 = m_Scene.CreateEntity();
	pLeg1.AddComponent<Tag>().Name = "Player Leg FR";
	auto& tc = pLeg1.AddComponent<Transform>();
	auto& kin = pLeg1.AddComponent<Kinematics>();
    tc.LocalPosition = {0.6f,0.0f,-0.6f};
	pLeg1.AddComponent<Parent>().Parent = player;
    pLeg1.AddComponent<MeshRenderer>().MeshPtr = m_PlayerLegMesh.get();
    auto& pLegAnim1 = pLeg1.AddComponent<MeshAnimation>();
	// Strafe -> x
	auto& pLeg1XX = pLegAnim1.Links[ (int)MotionAxis::X ][ (int)MotionAxis::X ];
	pLeg1XX.Enabled = true;
	pLeg1XX.Magnitude = -1.0f *0.01f;
	pLeg1XX.Frequency = 1.6f;
	pLeg1XX.Damping = 0.6f;
	pLeg1XX.Response = 1.4f;
	pLeg1XX.Clamp = 45.0f;
	// Forward -> z
	auto& pLeg1ZZ = pLegAnim1.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	pLeg1ZZ.Enabled = true;
	pLeg1ZZ.Magnitude = -1.0f *0.01f;
	pLeg1ZZ.Frequency = 1.6f;
	pLeg1ZZ.Damping = 0.6f;
	pLeg1ZZ.Response = 1.4f;
	pLeg1ZZ.Clamp = 45.0f;
	// Up -> y
	auto& pLeg1UU = pLegAnim1.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Y ];
	pLeg1UU.Enabled = true;
	pLeg1UU.Magnitude = -1.0f *0.01f;
	pLeg1UU.Frequency = 1.6f;
	pLeg1UU.Damping = 0.6f;
	pLeg1UU.Response = 1.4f;
	pLeg1UU.Clamp = 45.0f;
	// Up -> x
	auto& pLeg1UX = pLegAnim1.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::X ];
	pLeg1UX.Enabled = true;
	pLeg1UX.Magnitude = -1.0f *0.01f;
	pLeg1UX.Frequency = 1.6f;
	pLeg1UX.Damping = 0.6f;
	pLeg1UX.Response = 1.4f;
	pLeg1UX.Clamp = 45.0f;
	// Up -> z
	auto& pLeg1UZ = pLegAnim1.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Z ];
	pLeg1UZ.Enabled = true;
	pLeg1UZ.Magnitude = -1.0f *0.01f;
	pLeg1UZ.Frequency = 1.6f;
	pLeg1UZ.Damping = 0.6f;
	pLeg1UZ.Response = 1.4f;
	pLeg1UZ.Clamp = 45.0f;
	// Strafe -> roll
	auto& pLeg1XR = pLegAnim1.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	pLeg1XR.Enabled = true;
	pLeg1XR.Magnitude = -5.0f;
	pLeg1XR.Frequency = 1.6f;
	pLeg1XR.Damping = 0.45f;
	pLeg1XR.Response = 0.25f;
	pLeg1XR.Clamp = 45.0f;
	// Forward -> pitch
	auto& pLeg1ZP = pLegAnim1.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	pLeg1ZP.Enabled = true;
	pLeg1ZP.Magnitude = 5.0f;
	pLeg1ZP.Frequency = 1.6f;
	pLeg1ZP.Damping = 0.45f;
	pLeg1ZP.Response = 0.25f;
	pLeg1ZP.Clamp = 45.0f;
	// up -> roll
	auto& pLeg1UR = pLegAnim1.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Roll ];
	pLeg1UR.Enabled = true;
	pLeg1UR.Magnitude = -1.0f;
	pLeg1UR.Frequency = 1.6f;
	pLeg1UR.Damping = 0.6f;
	pLeg1UR.Response = 1.4f;
	pLeg1UR.Clamp = 45.0f;
	// up -> pitch
	auto& pLeg1UP = pLegAnim1.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Pitch ];
	pLeg1UP.Enabled = true;
	pLeg1UP.Magnitude = -1.0f;
	pLeg1UP.Frequency = 1.6f;
	pLeg1UP.Damping = 0.45f;
	pLeg1UP.Response = 0.25f;
	pLeg1UP.Clamp = 45.0f;
	// yaw -> roll
	auto& pLeg1YR = pLegAnim1.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Roll ];
	pLeg1YR.Enabled = true;
	pLeg1YR.Magnitude = 0.5f;
	pLeg1YR.Frequency = 1.6f;
	pLeg1YR.Damping = 0.6f;
	pLeg1YR.Response = 1.4f;
	pLeg1YR.Clamp = 45.0f;
	// yaw -> pitch
	auto& pLeg1YP = pLegAnim1.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Pitch ];
	pLeg1YP.Enabled = true;
	pLeg1YP.Magnitude = -0.5f;
	pLeg1YP.Frequency = 1.6f;
	pLeg1YP.Damping = 0.45f;
	pLeg1YP.Response = 0.25f;
	pLeg1YP.Clamp = 45.0f;
	}
	// Leg2 
	{
	auto pLeg2 = m_Scene.CreateEntity();
	pLeg2.AddComponent<Tag>().Name = "Player Leg BR";
	auto& tc = pLeg2.AddComponent<Transform>();
	auto& kin = pLeg2.AddComponent<Kinematics>();
    tc.LocalPosition = {0.6f,0.0f,0.6f};
    tc.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(-90.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	pLeg2.AddComponent<Parent>().Parent = player;
    pLeg2.AddComponent<MeshRenderer>().MeshPtr = m_PlayerLegMesh.get();
    auto& pLegAnim2 = pLeg2.AddComponent<MeshAnimation>();
	// Strafe -> x
	auto& pLeg2XX = pLegAnim2.Links[ (int)MotionAxis::X ][ (int)MotionAxis::X ];
	pLeg2XX.Enabled = true;
	pLeg2XX.Magnitude = -1.0f *0.01f;
	pLeg2XX.Frequency = 1.6f;
	pLeg2XX.Damping = 0.6f;
	pLeg2XX.Response = 1.4f;
	pLeg2XX.Clamp = 45.0f;
	// Forward -> z
	auto& pLeg2ZZ = pLegAnim2.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	pLeg2ZZ.Enabled = true;
	pLeg2ZZ.Magnitude = -1.0f *0.01f;
	pLeg2ZZ.Frequency = 1.6f;
	pLeg2ZZ.Damping = 0.6f;
	pLeg2ZZ.Response = 1.4f;
	pLeg2ZZ.Clamp = 45.0f;
	// Up -> y
	auto& pLeg2UU = pLegAnim2.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Y ];
	pLeg2UU.Enabled = true;
	pLeg2UU.Magnitude = -1.0f *0.01f;
	pLeg2UU.Frequency = 1.6f;
	pLeg2UU.Damping = 0.6f;
	pLeg2UU.Response = 1.4f;
	pLeg2UU.Clamp = 45.0f;
	// Up -> x
	auto& pLeg2UX = pLegAnim2.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::X ];
	pLeg2UX.Enabled = true;
	pLeg2UX.Magnitude = -1.0f *0.01f;
	pLeg2UX.Frequency = 1.6f;
	pLeg2UX.Damping = 0.6f;
	pLeg2UX.Response = 1.4f;
	pLeg2UX.Clamp = 45.0f;
	// Up -> z
	auto& pLeg2UZ = pLegAnim2.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Z ];
	pLeg2UZ.Enabled = true;
	pLeg2UZ.Magnitude = -1.0f *0.01f;
	pLeg2UZ.Frequency = 1.6f;
	pLeg2UZ.Damping = 0.6f;
	pLeg2UZ.Response = 1.4f;
	pLeg2UZ.Clamp = 45.0f;
	// Strafe -> roll
	auto& pLeg2XR = pLegAnim2.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	pLeg2XR.Enabled = true;
	pLeg2XR.Magnitude = -5.0f;
	pLeg2XR.Frequency = 1.6f;
	pLeg2XR.Damping = 0.6f;
	pLeg2XR.Response = 1.4f;
	pLeg2XR.Clamp = 45.0f;
	// Forward -> pitch
	auto& pLeg2ZP = pLegAnim2.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	pLeg2ZP.Enabled = true;
	pLeg2ZP.Magnitude = 5.0f;
	pLeg2ZP.Frequency = 1.6f;
	pLeg2ZP.Damping = 0.45f;
	pLeg2ZP.Response = 0.25f;
	pLeg2ZP.Clamp = 45.0f;
	// up -> roll
	auto& pLeg2UR = pLegAnim2.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Roll ];
	pLeg2UR.Enabled = true;
	pLeg2UR.Magnitude = -1.0f;
	pLeg2UR.Frequency = 1.6f;
	pLeg2UR.Damping = 0.45f;
	pLeg2UR.Response = 0.25f;
	pLeg2UR.Clamp = 45.0f;
	// up -> pitch
	auto& pLeg2UP = pLegAnim2.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Pitch ];
	pLeg2UP.Enabled = true;
	pLeg2UP.Magnitude = -1.0f;
	pLeg2UP.Frequency = 1.6f;
	pLeg2UP.Damping = 0.45f;
	pLeg2UP.Response = 0.25f;
	pLeg2UP.Clamp = 45.0f;
	// yaw -> roll
	auto& pLeg2YR = pLegAnim2.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Roll ];
	pLeg2YR.Enabled = true;
	pLeg2YR.Magnitude = 2.5f;
	pLeg2YR.Frequency = 1.6f;
	pLeg2YR.Damping = 0.6f;
	pLeg2YR.Response = 1.4f;
	pLeg2YR.Clamp = 45.0f;
	// yaw -> pitch
	auto& pLeg2YP = pLegAnim2.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Pitch ];
	pLeg2YP.Enabled = true;
	pLeg2YP.Magnitude = -2.5f;
	pLeg2YP.Frequency = 1.6f;
	pLeg2YP.Damping = 0.45f;
	pLeg2YP.Response = 0.25f;
	pLeg2YP.Clamp = 45.0f;
	}
	// Leg3 
	m_PlayerLegMeshMirrored = m_PlayerLegMesh->CreateMirrored(true,false,false);
	{
	auto pLeg3 = m_Scene.CreateEntity();
	pLeg3.AddComponent<Tag>().Name = "Player Leg FL";
	auto& tc = pLeg3.AddComponent<Transform>();
	auto& kin = pLeg3.AddComponent<Kinematics>();
    tc.LocalPosition = {-0.6f,0.0f,-0.6f};
	pLeg3.AddComponent<Parent>().Parent = player;
	auto& pmeshComp1 = pLeg3.AddComponent<MeshRenderer>();
	pmeshComp1.MeshPtr = m_PlayerLegMeshMirrored.get();
    auto& pLegAnim3 = pLeg3.AddComponent<MeshAnimation>();
	// Strafe -> x
	auto& pLeg3XX = pLegAnim3.Links[ (int)MotionAxis::X ][ (int)MotionAxis::X ];
	pLeg3XX.Enabled = true;
	pLeg3XX.Magnitude = -1.0f *0.01f;
	pLeg3XX.Frequency = 1.6f;
	pLeg3XX.Damping = 0.6f;
	pLeg3XX.Response = 1.4f;
	pLeg3XX.Clamp = 45.0f;
	// Forward -> z
	auto& pLeg3ZZ = pLegAnim3.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	pLeg3ZZ.Enabled = true;
	pLeg3ZZ.Magnitude = -1.0f *0.01f;
	pLeg3ZZ.Frequency = 1.6f;
	pLeg3ZZ.Damping = 0.6f;
	pLeg3ZZ.Response = 1.4f;
	pLeg3ZZ.Clamp = 45.0f;
	// Up -> y
	auto& pLeg3UU = pLegAnim3.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Y ];
	pLeg3UU.Enabled = true;
	pLeg3UU.Magnitude = -1.0f *0.01f;
	pLeg3UU.Frequency = 1.6f;
	pLeg3UU.Damping = 0.6f;
	pLeg3UU.Response = 1.4f;
	pLeg3UU.Clamp = 45.0f;
	// Up -> x
	auto& pLeg3UX = pLegAnim3.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::X ];
	pLeg3UX.Enabled = true;
	pLeg3UX.Magnitude = -1.0f *0.01f;
	pLeg3UX.Frequency = 1.6f;
	pLeg3UX.Damping = 0.6f;
	pLeg3UX.Response = 1.4f;
	pLeg3UX.Clamp = 45.0f;
	// Up -> z
	auto& pLeg3UZ = pLegAnim3.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Z ];
	pLeg3UZ.Enabled = true;
	pLeg3UZ.Magnitude = -1.0f *0.01f;
	pLeg3UZ.Frequency = 1.6f;
	pLeg3UZ.Damping = 0.6f;
	pLeg3UZ.Response = 1.4f;
	pLeg3UZ.Clamp = 45.0f;
	// Strafe -> roll
	auto& pLeg3XR = pLegAnim3.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	pLeg3XR.Enabled = true;
	pLeg3XR.Magnitude = -5.0f;
	pLeg3XR.Frequency = 1.6f;
	pLeg3XR.Damping = 0.45f;
	pLeg3XR.Response = 0.25f;
	pLeg3XR.Clamp = 45.0f;
	// Forward -> pitch
	auto& pLeg3ZP = pLegAnim3.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	pLeg3ZP.Enabled = true;
	pLeg3ZP.Magnitude = 5.0f;
	pLeg3ZP.Frequency = 1.6f;
	pLeg3ZP.Damping = 0.45f;
	pLeg3ZP.Response = 0.25f;
	pLeg3ZP.Clamp = 45.0f;
	// up -> roll
	auto& pLeg3UR = pLegAnim3.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Roll ];
	pLeg3UR.Enabled = true;
	pLeg3UR.Magnitude = -1.0f;
	pLeg3UR.Frequency = 1.6f;
	pLeg3UR.Damping = 0.6f;
	pLeg3UR.Response = 1.4f;
	pLeg3UR.Clamp = 45.0f;
	// up -> pitch
	auto& pLeg3UP = pLegAnim3.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Pitch ];
	pLeg3UP.Enabled = true;
	pLeg3UP.Magnitude = -1.0f;
	pLeg3UP.Frequency = 1.6f;
	pLeg3UP.Damping = 0.6f;
	pLeg3UP.Response = 1.4f;
	pLeg3UP.Clamp = 45.0f;
	// yaw -> roll
	auto& pLeg3YR = pLegAnim3.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Roll ];
	pLeg3YR.Enabled = true;
	pLeg3YR.Magnitude = 2.5f;
	pLeg3YR.Frequency = 1.6f;
	pLeg3YR.Damping = 0.45f;
	pLeg3YR.Response = 0.25f;
	pLeg3YR.Clamp = 45.0f;
	// yaw -> pitch
	auto& pLeg3YP = pLegAnim3.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Pitch ];
	pLeg3YP.Enabled = true;
	pLeg3YP.Magnitude = -2.5f;
	pLeg3YP.Frequency = 1.6f;
	pLeg3YP.Damping = 0.45f;
	pLeg3YP.Response = 0.25f;
	pLeg3YP.Clamp = 45.0f;
	}
	// Leg4 
	{
	auto pLeg4 = m_Scene.CreateEntity();
	pLeg4.AddComponent<Tag>().Name = "Player Leg BL";
	auto& tc = pLeg4.AddComponent<Transform>();
	auto& kin = pLeg4.AddComponent<Kinematics>();
    tc.LocalPosition = {-0.6f,0.0f,0.6f};
    tc.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(90.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	pLeg4.AddComponent<Parent>().Parent = player;
	auto& pmeshComp2 = pLeg4.AddComponent<MeshRenderer>();
	pmeshComp2.MeshPtr = m_PlayerLegMeshMirrored.get();
    auto& pLegAnim4 = pLeg4.AddComponent<MeshAnimation>();
	// Strafe -> x
	auto& pLeg4XX = pLegAnim4.Links[ (int)MotionAxis::X ][ (int)MotionAxis::X ];
	pLeg4XX.Enabled = true;
	pLeg4XX.Magnitude = -1.0f *0.01f;
	pLeg4XX.Frequency = 1.6f;
	pLeg4XX.Damping = 0.6f;
	pLeg4XX.Response = 1.4f;
	pLeg4XX.Clamp = 45.0f;
	// Forward -> z
	auto& pLeg4ZZ = pLegAnim4.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	pLeg4ZZ.Enabled = true;
	pLeg4ZZ.Magnitude = -1.0f *0.01f;
	pLeg4ZZ.Frequency = 1.6f;
	pLeg4ZZ.Damping = 0.6f;
	pLeg4ZZ.Response = 1.4f;
	pLeg4ZZ.Clamp = 45.0f;
	// Up -> y
	auto& pLeg4UU = pLegAnim4.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Y ];
	pLeg4UU.Enabled = true;
	pLeg4UU.Magnitude = -1.0f *0.01f;
	pLeg4UU.Frequency = 1.6f;
	pLeg4UU.Damping = 0.6f;
	pLeg4UU.Response = 1.4f;
	pLeg4UU.Clamp = 45.0f;
	// Up -> x
	auto& pLeg4UX = pLegAnim4.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::X ];
	pLeg4UX.Enabled = true;
	pLeg4UX.Magnitude = -1.0f *0.01f;
	pLeg4UX.Frequency = 1.6f;
	pLeg4UX.Damping = 0.6f;
	pLeg4UX.Response = 1.4f;
	pLeg4UX.Clamp = 45.0f;
	// Up -> z
	auto& pLeg4UZ = pLegAnim4.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Z ];
	pLeg4UZ.Enabled = true;
	pLeg4UZ.Magnitude = -1.0f *0.01f;
	pLeg4UZ.Frequency = 1.6f;
	pLeg4UZ.Damping = 0.6f;
	pLeg4UZ.Response = 1.4f;
	pLeg4UZ.Clamp = 45.0f;
	// Strafe -> roll
	auto& pLeg4XR = pLegAnim4.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	pLeg4XR.Enabled = true;
	pLeg4XR.Magnitude = -5.0f;
	pLeg4XR.Frequency = 1.6f;
	pLeg4XR.Damping = 0.45f;
	pLeg4XR.Response = 0.25f;
	pLeg4XR.Clamp = 45.0f;
	// Forward -> pitch
	auto& pLeg4ZP = pLegAnim4.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
	pLeg4ZP.Enabled = true;
	pLeg4ZP.Magnitude = 5.0f;
	pLeg4ZP.Frequency = 1.6f;
	pLeg4ZP.Damping = 0.45f;
	pLeg4ZP.Response = 0.25f;
	pLeg4ZP.Clamp = 45.0f;
	// up -> roll
	auto& pLeg4UR = pLegAnim4.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Roll ];
	pLeg4UR.Enabled = true;
	pLeg4UR.Magnitude = -1.0f;
	pLeg4UR.Frequency = 1.6f;
	pLeg4UR.Damping = 0.6f;
	pLeg4UR.Response = 1.4f;
	pLeg4UR.Clamp = 45.0f;
	// up -> pitch
	auto& pLeg4UP = pLegAnim4.Links[ (int)MotionAxis::Y ][ (int)MotionAxis::Pitch ];
	pLeg4UP.Enabled = true;
	pLeg4UP.Magnitude = -1.0f;
	pLeg4UP.Frequency = 1.6f;
	pLeg4UP.Damping = 0.6f;
	pLeg4UP.Response = 1.4f;
	pLeg4UP.Clamp = 45.0f;
	// yaw -> roll
	auto& pLeg4YR = pLegAnim4.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Roll ];
	pLeg4YR.Enabled = true;
	pLeg4YR.Magnitude = 2.5f;
	pLeg4YR.Frequency = 1.6f;
	pLeg4YR.Damping = 0.45f;
	pLeg4YR.Response = 0.25f;
	pLeg4YR.Clamp = 45.0f;
	// yaw -> pitch
	auto& pLeg4YP = pLegAnim4.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Pitch ];
	pLeg4YP.Enabled = true;
	pLeg4YP.Magnitude = -2.5f;
	pLeg4YP.Frequency = 1.6f;
	pLeg4YP.Damping = 0.45f;
	pLeg4YP.Response = 0.25f;
	pLeg4YP.Clamp = 45.0f;
	}






    // PLAYER Gun ENTITY
	auto gun1 = m_Scene.CreateEntity();
	gun1.AddComponent<Tag>().Name = "Gun1";
	auto& gt1 = gun1.AddComponent<Transform>();
	auto& king1 = gun1.AddComponent<Kinematics>();
    gt1.LocalPosition = {0.06f ,-0.06f ,-0.8f};
	gun1.AddComponent<MeshRenderer>().MeshPtr = m_GunMesh.get();
	gun1.AddComponent<Parent>().Parent = player;
    auto& gunAnim1 = gun1.AddComponent<MeshAnimation>();
	// Forward velocity -> pitch
	auto& Gun1StrafeRoll = gunAnim1.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
	Gun1StrafeRoll.Enabled = true;
	Gun1StrafeRoll.Magnitude = -1.2f;
	Gun1StrafeRoll.Frequency = 1.8f;
	Gun1StrafeRoll.Damping = 0.8f;
	Gun1StrafeRoll.Response = 2.0f;
	Gun1StrafeRoll.Clamp = 45.0f;
	auto& Gun1ForwardLag = gunAnim1.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
	Gun1ForwardLag.Enabled = true;
	Gun1ForwardLag.Magnitude = 0.002f;
	Gun1ForwardLag.Frequency = 1.8f;
	Gun1ForwardLag.Damping = 0.8f;
	Gun1ForwardLag.Response = 2.0f;
	Gun1ForwardLag.Clamp = 45.0f;
	auto& Gun1YawYaw = gunAnim1.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Yaw ];
	Gun1YawYaw.Enabled = true;
	Gun1YawYaw.Magnitude = 5.0f;
	Gun1YawYaw.Frequency = 1.8f;
	Gun1YawYaw.Damping = 1.3f;
	Gun1YawYaw.Response = 0.5f;
	Gun1YawYaw.Clamp = 45.0f;
	auto& Gun1PitchPitch = gunAnim1.Links[ (int)MotionAxis::Pitch ][ (int)MotionAxis::Pitch ];
	Gun1PitchPitch.Enabled = true;
	Gun1PitchPitch.Magnitude = 3.5f;
	Gun1PitchPitch.Frequency = 1.8f;
	Gun1PitchPitch.Damping = 1.3f;
	Gun1PitchPitch.Response = 0.5f;
	Gun1PitchPitch.Clamp = 45.0f;


    // CAMERA ENTITY
    auto camEntity = m_Scene.CreateEntity();
	camEntity.AddComponent<Tag>().Name = "Player Camera";
    camEntity.AddComponent<Transform>();
	camEntity.AddComponent<Kinematics>();
    camEntity.AddComponent<CameraComponent>();
    camEntity.AddComponent<Parent>().Parent = player;
    camEntity.LocalPosition = {0.0f, 0.3f , 3.35f};
    camEntity.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));


	// Collider
	auto& collider = player.AddComponent<AABBCollider>();
	collider.HalfSize = {0.5f, 0.5f, 0.5f};
	auto& rb = player.AddComponent<Rigidbody>();
	auto& m = player.AddComponent<Movement>();
	rb.Velocity = {0,0,0};
	rb.IsStatic = false;	




	// Enemy
    auto enemy = m_Scene.CreateEntity();
	enemy.AddComponent<Tag>().Name = "Enemy";
	auto& et = enemy.AddComponent<Transform>();
	auto& eKin = enemy.AddComponent<Kinematics>();
    et.LocalPosition = {2,1,0};

	// Body 
	auto ebody = m_Scene.CreateEntity();
	{
	ebody.AddComponent<Tag>().Name = "Enemy Body";
	auto& tc = ebody.AddComponent<Transform>();
	auto& eKin = ebody.AddComponent<Kinematics>();
    tc.LocalPosition = {0.0f,0.0f,0.0f};
	ebody.AddComponent<Parent>().Parent = enemy;
    ebody.AddComponent<MeshRenderer>().MeshPtr = m_EnemyBodyMesh.get();
	}

	// Leg1 
	{
	auto eLeg1 = m_Scene.CreateEntity();
	eLeg1.AddComponent<Tag>().Name = "Enemy Leg FL";
	auto& tc = eLeg1.AddComponent<Transform>();
	auto& eKin = eLeg1.AddComponent<Kinematics>();
    tc.LocalPosition = {0.6f,0.0f,-0.6f};
	eLeg1.AddComponent<Parent>().Parent = enemy;
    eLeg1.AddComponent<MeshRenderer>().MeshPtr = m_EnemyLegMesh.get();
	}
	// Leg2 
	{
	auto eLeg2 = m_Scene.CreateEntity();
	eLeg2.AddComponent<Tag>().Name = "Enemy Leg BL";
	auto& tc = eLeg2.AddComponent<Transform>();
	auto& eKin = eLeg2.AddComponent<Kinematics>();
    tc.LocalPosition = {0.6f,0.0f,0.6f};
    tc.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(-90.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	eLeg2.AddComponent<Parent>().Parent = enemy;
    eLeg2.AddComponent<MeshRenderer>().MeshPtr = m_EnemyLegMesh.get();
	}
	// Leg3 
	m_EnemyLegMeshMirrored = m_EnemyLegMesh->CreateMirrored(true,false,false);
	{
	auto eLeg3 = m_Scene.CreateEntity();
	eLeg3.AddComponent<Tag>().Name = "Enemy Leg FR";
	auto& tc = eLeg3.AddComponent<Transform>();
	auto& eKin = eLeg3.AddComponent<Kinematics>();
    tc.LocalPosition = {-0.6f,0.0f,-0.6f};
	eLeg3.AddComponent<Parent>().Parent = enemy;
	auto& meshComp1 = eLeg3.AddComponent<MeshRenderer>();
	meshComp1.MeshPtr = m_EnemyLegMeshMirrored.get();
	}
	// Leg4 
	{
	auto eLeg4 = m_Scene.CreateEntity();
	eLeg4.AddComponent<Tag>().Name = "Enemy Leg BR";
	auto& tc = eLeg4.AddComponent<Transform>();
	auto& eKin = eLeg4.AddComponent<Kinematics>();
    tc.LocalPosition = {-0.6f,0.0f,0.6f};
    tc.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(90.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	eLeg4.AddComponent<Parent>().Parent = enemy;
	auto& meshComp2 = eLeg4.AddComponent<MeshRenderer>();
	meshComp2.MeshPtr = m_EnemyLegMeshMirrored.get();
	}
	// Collider
	{
	auto& collider = enemy.AddComponent<AABBCollider>();
	collider.HalfSize = {0.5f, 0.5f, 0.5f};
	auto& rb = enemy.AddComponent<Rigidbody>();
	auto& m = enemy.AddComponent<Movement>();
	rb.Velocity = {0,0,0};
	rb.IsStatic = false;	
	}




    // RANDOM CUBES 
	int numCubes = 20;
	float spawnRange = 10.f;
    for (int i = 0; i < numCubes; i++) {
        auto e = m_Scene.CreateEntity();
		e.AddComponent<Tag>().Name = "Cube";

        auto& t = e.AddComponent<Transform>();
		float X = RandomFloat() * spawnRange;
		float Y = 20 + RandomFloat() * spawnRange;
		float Z = RandomFloat() * spawnRange;
        t.LocalPosition = {X, Y, Z};
        t.LocalOrientation = 
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(1,0,0)) *
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0,1,0)) *
    		glm::angleAxis(glm::radians(RandomFloat() * 180.f), glm::vec3(0,0,1));
		e.AddComponent<MeshRenderer>().MeshPtr = m_CubeMesh.get();

        auto& rb = e.AddComponent<Rigidbody>();
        rb.IsStatic = true;
	
        // Collider
        auto& collider = e.AddComponent<AABBCollider>();
        collider.HalfSize = {0.5f, 0.5f, 0.5f};
    }

    // WORLD 
	auto b = m_Scene.CreateEntity();
	b.AddComponent<Tag>().Name = "World";
    auto& tb = b.AddComponent<Transform>();
    tb.LocalPosition = {0.0f, -25.0f, 0.0f};
    tb.LocalOrientation = 
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(1,0,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,1,0)) *
    	glm::angleAxis(glm::radians(0.0f), glm::vec3(0,0,1));
	b.AddComponent<MeshRenderer>().MeshPtr = m_BoxMesh.get();
    auto& rb_box = b.AddComponent<Rigidbody>();
    rb_box.IsStatic = true;
    //auto& worldcollider = b.AddComponent<AABBCollider>();
    //worldcollider.HalfSize = {100.0f, 25.0f, 100.0f}; // There are bugs with large colliders. 
	

	// DEFAULT FOG
	m_Fog.Color = {0.78f, 0.98f, 1.0f};
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

	auto camView = m_Scene.Registry().view<CameraComponent>();
	for (auto entity : camView) {
    	auto& cameraComp = playerView.get<CameraComponent>(entity);
		if (cameraComp.Primary) {
    		m_Scene.OnUpdate(dt, cameraComp);
		}
	}
	
	auto playerView = m_Scene.Registry().view<Transform, PlayerController>();

	for (auto entity : playerView) {
    	auto& playerTransform = playerView.get<Transform>(entity);

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
	
	// Cube click test
	static bool lastClick = false;

	bool click = Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || ControllerInput::IsButtonPressed(0, GamepadButton::R1);
	for (int i = 0; i < 16; i++) {
	    if (ControllerInput::IsButtonPressed(0, (GamepadButton)i)) {
	        WK_CORE_INFO("BUTTON PRESSED: {0}", i);
	    }
	}

	if (click && !lastClick) {
		Ray ray = CreateCameraRay(cam, camPos);
		for (auto entity : playerView) {
    		auto& playerTransform = playerView.get<Transform>(entity);
			ray.Direction = playerTransform.LocalOrientation * glm::vec3(0, 0, -1);
			ray.Origin = playerTransform.LocalPosition + glm::normalize(ray.Direction)*0.7f;
		}

		RaycastHit hit;
		float maxDist = 1000.0f;

		if (RaycastAABB(m_Scene, ray, hit, maxDist)) {
			Entity e = hit.HitEntity;

			auto& registry = m_Scene.Registry();

			if (registry.all_of<Tag>(e.GetHandle())) {
				auto& tag = registry.get<Tag>(e.GetHandle());

				WK_CORE_INFO("Ray hit entity: {0}", tag.Name);

				// ONLY teleport cubes
				if (tag.Name == "Cube") {
				    float range = 10.0f;
				    glm::vec3 newPos(RandomFloat() * range, 20.f+RandomFloat() * range, RandomFloat() * range);
				    registry.get<Transform>(e.GetHandle()).LocalPosition = newPos;
				}
			}
		}
	}

	lastClick = click;
	
	

	auto debugView = m_Scene.Registry().view<Transform, MeshRenderer>();
	auto colliderView = m_Scene.Registry().view<Transform, AABBCollider>();
	//auto colliderView = m_Scene.Registry().view<Transform, Collider>();
	auto view = m_Scene.Registry().view<Transform, MeshRenderer>();

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
						if (d < -0.05f)
						    continue;
					}
				}

				// Render
				for (auto entity : view) {
					auto& transform = view.get<Transform>(entity);
    			    auto& mesh = view.get<MeshRenderer>(entity);
					glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform * mesh.GetLocalTransform();

					Renderer::Submit(model, *mesh.MeshPtr, m_Shader.get());
				}

				// DEBUG AXES
				if (Renderer::DebugEnabled) {
					for (auto entity : debugView) {
						auto& transform = debugView.get<Transform>(entity);
						glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform;
				    	glm::vec3 origin = glm::vec3(model[3]);
				    	float axisLength = 0.25f;
				
				    	glm::vec3 right = glm::normalize(glm::vec3(model[0]));
				    	glm::vec3 up = glm::normalize(glm::vec3(model[1]));
				    	glm::vec3 forward = glm::normalize(glm::vec3(model[2]));
				
				    	std::vector<DebugLine> lines = {
				    	    { origin, origin + right * axisLength,   {0.8, 0.3, 0.0} },// X axis (Right)
				    	    { origin, origin + up * axisLength,      {0.6, 1.0, 0.0} },// Y axis (Up)
				    	    { origin, origin + forward * axisLength, {0.4, 0.0, 0.9} } // Z axis (Backward)
				    	};
						
				    	// Parent link
				    	if (m_Scene.Registry().all_of<Parent>(entity)) {
				    	    auto parent = m_Scene.Registry().get<Parent>(entity).Parent;
				    	    if (parent) {
								auto& childTransform = m_Scene.Registry().get<Transform>(entity);
				    	        auto& parentTransform = parent.GetComponent<Transform>();
								glm::vec3 childPos = glm::vec3(childTransform.WorldTransform[3]);
				    	        glm::vec3 parentPos = glm::vec3(parentTransform.WorldTransform[3]);
				    	        lines.push_back({childPos, parentPos, {1,1,1}});
				    	    }
				    	}
				    	Renderer::SubmitDebugLines(lines);
					}

					// Collider Debug
					for (auto entity : colliderView) {
						auto& transform = colliderView.get<Transform>(entity);
						glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform;
    					auto& collider = colliderView.get<AABBCollider>(entity);
				    	glm::vec3 origin = glm::vec3(model[3]);
				    	float axisLength = 0.25f;
				
				    	glm::vec3 right = glm::normalize(glm::vec3(model[0]));
				    	glm::vec3 up = glm::normalize(glm::vec3(model[1]));
				    	glm::vec3 forward = glm::normalize(glm::vec3(model[2]));
				
				    	std::vector<DebugLine> lines = {};
						
						glm::vec3 cDims = collider.HalfSize;
				    	glm::vec3 cOrigin = glm::vec3(model[3]) + collider.Offset;
						//glm::vec3 colliderLineColor = {0.6, 1.0, 0.0};
						glm::vec3 colliderLineColor = {0.4f, 0.0f, 1.0f};
						// Right
						lines.push_back({ cOrigin + right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  cOrigin + right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  cOrigin + right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  cOrigin + right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  cOrigin + right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						// Left
						lines.push_back({ cOrigin - right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  cOrigin - right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin - right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  cOrigin - right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin - right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  cOrigin - right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin - right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  cOrigin - right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						// Connection lines
						lines.push_back({ cOrigin + right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  cOrigin - right*cDims[0] + up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  cOrigin - right*cDims[0] + up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  cOrigin - right*cDims[0] - up*cDims[1] + forward*cDims[2], 
						                  colliderLineColor });
						lines.push_back({ cOrigin + right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  cOrigin - right*cDims[0] - up*cDims[1] - forward*cDims[2], 
						                  colliderLineColor });
				
				    	Renderer::SubmitDebugLines(lines);
					}
				}

			} // Z
		} // Y
	} // X

	Renderer::EndScene();
}


void SandboxLayer::OnImGuiRender() {
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->GetGlyphRangesDefault();
	//if (!m_GameFocused || m_GameFocused) {// making this always on now. Better tesing
	if (!m_GameFocused) {
		ImGui::Begin("Renderer Debug");
		DebugOverlay::DrawFPSPanel();
		
		// FOG
		if (ImGui::CollapsingHeader("Fog")) { 
			//if (ImGui::CollapsingHeader("Color Picker")) { 
			//	ImGui::SameLine();
			//	ImGui::ColorPicker3("##FogColor", 
			//		&m_Fog.Color[0], 
			//		ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float
			//	);
			//}
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
			
			// TODO: Need to update this with new camera stuff
			//glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(m_DebugFollow->RotationOffset));
			//if (m_DebugFollow) {
			//    ImGui::Separator();
			//    ImGui::Text("Follow Camera");
			//    ImGui::DragFloat3("Offset", &m_DebugFollow->Offset[0], 0.01f);
			//    ImGui::DragFloat3("Rotation Offset [pyr]", &eulerDeg[0], 0.1f);
			//	glm::vec3 eulerRad = glm::radians(eulerDeg);
    		//	m_DebugFollow->RotationOffset = glm::quat(eulerRad);
			//}
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
				if (registry.all_of<Tag>(entity))
				    name = registry.get<Tag>(entity).Name;
				//storageNames.push_back(name + " (" + std::to_string((uint32_t)entity) + ")");
				storageNames.push_back( "[" + std::to_string((uint32_t)entity) + "]  |  " + name);
			}

			for (auto& s : storageNames)
				labels.push_back(s.c_str());

			static int selectedIndex = 0;

			if (!labels.empty()) {
				if (ImGui::Combo("Selected", &selectedIndex, labels.data(), (int)labels.size())) {
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
				else if (!registry.all_of<MeshAnimation>(m_SelectedAnimEntity)) {
				    // NO COMPONENT → ADD BUTTON
				    if (ImGui::Button("Add MeshAnimation")) {
				        registry.emplace<MeshAnimation>(m_SelectedAnimEntity);
				    }
				}
				else {
				    // EDIT EXISTING COMPONENT
				    auto& anim = registry.get<MeshAnimation>(m_SelectedAnimEntity);
				    static int selectedOutputAxis[MeshAnimation::AxisCount] = {};
				    for (int input = 0; input < MeshAnimation::AxisCount; input++) {
				        MotionAxis inAxis = (MotionAxis)input;

				        if (!ImGui::TreeNode(MotionAxisName(inAxis)))
				            continue;

				        bool anyShown = false;
				        for (int output = 0; output < MeshAnimation::AxisCount; output++) {
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
								auto values = Wankel::SecondOrderPreview::GetStepResponse(link.Frequency, link.Damping, link.Response);
								ImGui::PlotLines("Step Response", values.data(), (int)values.size(),
								    0,
								    nullptr,
								    -0.5f,
								    2.0f,
								    ImVec2(0, 100)
								);

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
				        ImGui::Combo(comboId.c_str(), &selectedOutputAxis[input], MotionAxisLabels, MeshAnimation::AxisCount);

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
