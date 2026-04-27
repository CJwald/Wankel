#pragma once

#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/CameraController.h>
#include <Wankel/ECS/Scene.h>

namespace Wankel {

class SandboxLayer : public Layer {
public:
	SandboxLayer();
	virtual void OnUpdate() override;

private:
	unsigned int m_VAO = 0, m_VBO = 0;
	Shader* m_Shader = nullptr;

	CameraController m_Controller;
	Scene m_Scene;

	float m_LastFrame = 0.0f;
};

}
