#pragma once

#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/CameraController.h>
#include <Wankel/ECS/Scene.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/ECS/Components.h>
#include <memory>

namespace Wankel {

class VertexArray;
class VertexBuffer;
class Mesh;
	
class SandboxLayer : public Layer {
public:
	SandboxLayer();
	virtual void OnUpdate() override;
	virtual void OnEvent(Event& e) override;
	virtual void OnImGuiRender() override;

private:

	std::unique_ptr<Mesh> m_ShipMesh;
	std::unique_ptr<Mesh> m_CubeMesh;
	std::unique_ptr<Mesh> m_TriangleMesh;
	std::unique_ptr<Shader> m_Shader;

	FollowCameraComponent* m_DebugFollow = nullptr;
	CameraController m_Controller;
	Scene m_Scene;

	float m_LastFrame = 0.0f;

	// =========================
	// IMGUI / DEBUG
	// =========================

	// CHANGE: persistent fog settings
	FogSettings m_Fog;

	// CHANGE: whether user is controlling game
	bool m_GameFocused = true;
};

}
