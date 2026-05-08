#pragma once

#include <glm/glm.hpp>

namespace Wankel {

	class Camera;
	class Shader;
	class Mesh;

	struct FogSettings {
		glm::vec3 Color = {0.12f, 0.1f, 0.2f};
	
	    float Density = 0.01f;
	
	    // Noise fog
	    bool NoiseEnabled = true;
	
	    float NoiseScale = 0.05f;
	    float NoiseStrength = 0.75f;
	    float NoiseSpeed = 0.2f;
	
	    int NoiseOctaves = 4;
	
	    // Height fog
	    float HeightFalloff = 0.0f;
	};
	
	class Renderer {
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader);
		static void Draw(const Mesh& mesh);
	    static void Clear();
		static void OnWindowResize(uint32_t width, uint32_t height);
		static void SetFog(const FogSettings& fog);

	};

}
