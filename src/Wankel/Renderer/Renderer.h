#pragma once

namespace Wankel {

	class VertexArray;
	
	class Renderer {
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera camera);
		static void EndScene();
		static void Submit(const glm::mat4& transform, VertexArray* vao, Shader* shader);
	    static void Clear();
		static void Draw(VertexArray* vao, int count);
	};

}
