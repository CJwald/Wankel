#include "Player.h"

#include "MeshLoader.h"
#include "Wankel/Core/Application.h"
#include <glm/gtc/quaternion.hpp>

namespace Wankel {

void Player::Create(Scene& scene) {
    Build(scene);
}

void Player::Build(Scene& scene) {
    // LOAD SHARED PLAYER ASSETS
    m_ShipMesh = MeshLoader::Load("Assets/Mesh/SHIP05.ply");
    m_GunMesh  = MeshLoader::Load("Assets/Mesh/AK74_IRONS.ply");

    // ROOT PLAYER
    m_Player = scene.CreateEntity();
    m_Player.AddComponent<Tag>().Name = "Player";

    auto& pt = m_Player.AddComponent<Transform>();
    pt.LocalPosition = {0, 1, 0};

    m_Player.AddComponent<PlayerController>();
    m_Player.AddComponent<AABBCollider>().HalfSize = {0.5f, 0.5f, 0.5f};

    auto& rb = m_Player.AddComponent<Rigidbody>();
    rb.IsStatic = false;

    auto MakeShip = [&](glm::vec3 offset, const char* name, bool Mirrored) {
        auto ship = scene.CreateEntity();
        ship.AddComponent<Tag>().Name = name;

        auto& t = ship.AddComponent<Transform>();
        t.LocalPosition = offset;

		if (Mirrored) {
    		m_ShipMeshMirrored = m_ShipMesh->CreateMirrored(true, false, false);
    		ship.AddComponent<MeshRenderer>().MeshPtr = m_ShipMeshMirrored.get();
		}
		else {
        	ship.AddComponent<MeshRenderer>().MeshPtr = m_ShipMesh.get();
		}
        ship.AddComponent<Parent>().Parent = m_Player;

        auto& anim = ship.AddComponent<MeshAnimation>();
		// Forward velocity -> pitch
		auto& ForwardPitch = anim.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Pitch ];
		ForwardPitch.Enabled = true;
		ForwardPitch.Magnitude = 0.2f;
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

        return ship;
    };

    // LEFT SHIP / GUN BODY
    m_ShipLeft = MakeShip({-0.4f, 0.0f, 0.4f}, "ShipL", false);
    m_ShipRight = MakeShip({0.4f, 0.0f, 0.4f}, "ShipR", true);
    //auto shipL = scene.CreateEntity();
    //shipL.AddComponent<Tag>().Name = "ShipL";

    //auto& slT = shipL.AddComponent<Transform>();
    //slT.LocalPosition = {-0.4f, 0.0f, 0.4f};
    //shipL.AddComponent<Parent>().Parent = m_Player;
    //shipL.AddComponent<MeshRenderer>().MeshPtr = m_ShipMesh.get();

    //auto& animL = shipL.AddComponent<MeshAnimation>();



    // RIGHT SHIP (MIRRORED)
    //auto shipR = scene.CreateEntity();
    //shipR.AddComponent<Tag>().Name = "ShipR";

    //auto& srT = shipR.AddComponent<Transform>();
    //srT.LocalPosition = {0.4f, 0.0f, 0.4f};

    //shipR.AddComponent<Parent>().Parent = m_Player;

    //m_ShipMeshMirrored = m_ShipMesh->CreateMirrored(true, false, false);
    //shipR.AddComponent<MeshRenderer>().MeshPtr = m_ShipMeshMirrored.get();

    //m_GunRight = shipR;

    // GUNS
    auto MakeGun = [&](glm::vec3 offset, const char* name) {
        auto gun = scene.CreateEntity();
        gun.AddComponent<Tag>().Name = name;

        auto& t = gun.AddComponent<Transform>();
        t.LocalPosition = offset;

        gun.AddComponent<MeshRenderer>().MeshPtr = m_GunMesh.get();
        gun.AddComponent<Parent>().Parent = m_Player;

        auto& anim = gun.AddComponent<MeshAnimation>();
		// Forward velocity -> pitch
		auto& GunStrafeRoll = anim.Links[ (int)MotionAxis::X ][ (int)MotionAxis::Roll ];
		GunStrafeRoll.Enabled = true;
		GunStrafeRoll.Magnitude = -1.2f;
		GunStrafeRoll.Frequency = 1.8f;
		GunStrafeRoll.Damping = 0.8f;
		GunStrafeRoll.Response = 2.0f;
		GunStrafeRoll.Clamp = 8.0f;
		auto& GunForwardLag = anim.Links[ (int)MotionAxis::Z ][ (int)MotionAxis::Z ];
		GunForwardLag.Enabled = true;
		GunForwardLag.Magnitude = 0.002f;
		GunForwardLag.Frequency = 1.8f;
		GunForwardLag.Damping = 0.8f;
		GunForwardLag.Response = 2.0f;
		GunForwardLag.Clamp = 8.0f;
		auto& GunYawYaw = anim.Links[ (int)MotionAxis::Yaw ][ (int)MotionAxis::Yaw ];
		GunYawYaw.Enabled = true;
		GunYawYaw.Magnitude = 1.2f;
		GunYawYaw.Frequency = 1.8f;
		GunYawYaw.Damping = 0.8f;
		GunYawYaw.Response = 2.0f;
		GunYawYaw.Clamp = 8.0f;
		auto& GunPitchPitch = anim.Links[ (int)MotionAxis::Pitch ][ (int)MotionAxis::Pitch ];
		GunPitchPitch.Enabled = true;
		GunPitchPitch.Magnitude = 1.2f;
		GunPitchPitch.Frequency = 1.8f;
		GunPitchPitch.Damping = 0.8f;
		GunPitchPitch.Response = 2.0f;
		GunPitchPitch.Clamp = 8.0f;

        return gun;
    };

    m_GunLeft  = MakeGun({ 0.05f, 0.08f, -0.125f }, "GunL");
    m_GunRight = MakeGun({-0.05f, 0.08f, -0.125f }, "GunR");

    // CAMERA
    m_Camera = scene.CreateEntity();
    m_Camera.AddComponent<Tag>().Name = "Player Camera";

    auto& camT = m_Camera.AddComponent<Transform>();


    float pitch = 0, yaw = 0, roll = 0;
}

}
