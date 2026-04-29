#pragma once

#include <glm/glm.hpp>

namespace Wankel {

	class Camera;
	class Shader;
	class Mesh;
	
	class Renderer {
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader);
		static void Draw(const Mesh& mesh);
	    static void Clear(float r, float g, float b, float a);
		static void OnWindowResize(uint32_t width, uint32_t height);
	};

}
