#pragma once

#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/VertexArray.h>
#include <Wankel/Renderer/Buffer.h>
#include <Wankel/Renderer/CameraController.h>
#include <Wankel/ECS/Scene.h>

namespace Wankel {

class SandboxLayer : public Layer {
public:
	SandboxLayer();
	virtual void OnUpdate() override;

private:
	#include <memory>

	std::unique_ptr<VertexArray> m_VertexArray;
	std::unique_ptr<VertexBuffer> m_VertexBuffer;
	std::unique_ptr<Shader> m_Shader;

	CameraController m_Controller;
	Scene m_Scene;

	float m_LastFrame = 0.0f;
};

}
