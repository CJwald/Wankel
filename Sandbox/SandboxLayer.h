#pragma once

#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/CameraController.h>
#include <Wankel/ECS/Scene.h>
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

private:

	std::unique_ptr<Mesh> m_CubeMesh;
	std::unique_ptr<Mesh> m_TriangleMesh;
	std::unique_ptr<Shader> m_Shader;

	CameraController m_Controller;
	Scene m_Scene;

	float m_LastFrame = 0.0f;
};

}
