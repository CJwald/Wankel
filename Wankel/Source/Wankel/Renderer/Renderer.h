#pragma once

#include "../Core/Image.h"

#include "Camera.h"
#include "Ray.h"
#include "../Scene/Scene.h"
#include "Triangle.h"

#include <memory>
#include <glm/glm.hpp>

class Renderer
{
public:
	struct Settings {
		bool Accumulate = true;
		int maxRecursionDepth = 3;
		int bounces = 5;
		float maxRayTravelDist = 10000.0f;
		float fogdensity = 1.0f;
	};
public:
	Renderer() = default;

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera& camera);

	std::shared_ptr<Wankel::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }
private:
	struct HitPayload {
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		int ObjectIndex;
	};

	glm::vec4 PerPixel(uint32_t x, uint32_t y); // RayGen

	HitPayload TraceRay(Ray& ray, int depth, glm::vec3 centerFov, float farthestB);
	HitPayload ClosestHit(const Ray& ray, float hitDistance, int objectIndex);
	HitPayload Miss(const Ray& ray);
	/*glm::vec3 ComputeBRDF(const glm::vec3& albedo, float metallic, float roughness,
		const glm::vec3& normal, const glm::vec3& viewDir,
		const glm::vec3& lightDir, const glm::vec3& lightColor);*/

	bool TriangleHit(Ray& ray, Triangle& triangle);
private:
	std::shared_ptr<Wankel::Image> m_FinalImage;
	Settings m_Settings;

	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	uint32_t* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;
};