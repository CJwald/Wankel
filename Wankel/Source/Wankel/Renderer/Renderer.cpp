#include "Renderer.h"

#include "../Core/Random.h"
//#include <iostream>
#include <execution>


namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color) {
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

	static uint32_t PCG_Hash(uint32_t input) {
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed) {
		seed = PCG_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();
	}

	static glm::vec3 InUnitSphere(uint32_t& seed) {
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f)
		);
	}
}


void Renderer::OnResize(uint32_t width, uint32_t height) {
	if (m_FinalImage) {
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return; // No resize necessary
		m_FinalImage->Resize(width, height);
	} else {
		m_FinalImage = std::make_shared<Wankel::Image>(width, height, Wankel::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera) {
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;
	
	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y) {
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x) {
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor /= (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
				});
		});
#else
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++) {
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData);
	m_FrameIndex++;

	//if (m_Settings.Accumulate)
	//	m_FrameIndex++;
	//else
	//	m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y) {
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
	glm::vec3 centerFov = m_ActiveCamera->GetDirection();

	ray.Traveled = 0.0f;
	
	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);
	//glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
	//glm::vec3 skyColor = glm::vec3(0.5f, 0.7f, 0.2f);
	glm::vec3 skyColor = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 fogColor = glm::vec3(1.0f, 1.0f, 1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	//int bounces = 5;
	int recursionDepth = 0;
	for (int i = 0; i < m_Settings.bounces; i++) {
		seed += i;
		float farthestB = 0.0f;
		Renderer::HitPayload payload = TraceRay(ray, recursionDepth, centerFov, farthestB);
		if (payload.HitDistance < 0.0f) { 
			//light += skyColor * contribution;
			contribution *= skyColor; // maybe this insetad of above light equation?
			light += contribution;
			break;
		}

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		//contribution *= material.Albedo;
		contribution *= material.Albedo * (1.0f - material.Metallic); // Grok told me this might be a way to add metallic
		//light += (material.GetEmission() * contribution);
		//light += material.GetEmission();
		//float fogVisibilityScale = 2^(-(ray.Traveled*m_Settings.fogdensity)^2);
		light += contribution; // maybe? 
		light += material.GetEmission();
		
		//light += (material.GetEmission() * contribution);//ray.Traveled );

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		//ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
		ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed) * material.Roughness);
	}

	return glm::vec4(light, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(Ray& ray, int depth, glm::vec3 centerFov, float lastB) {
	// (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance
	depth += 1;
	float closestB = 0.0f;
	float farthestB = 0.0f;
	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();

	for (size_t j = 0; j < m_ActiveScene->Borders.size(); j++) {
		const BoundingSphere& border = m_ActiveScene->Borders[j];
		for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++) {
			const Sphere& sphere = m_ActiveScene->Spheres[i];
			glm::vec3 origin = ray.Origin - sphere.Position;

			float a = glm::dot(ray.Direction, ray.Direction);
			float b = 2.0f * glm::dot(origin, ray.Direction);
			float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

			// Quadratic forumula discriminant:
			// b^2 - 4ac

			float discriminant = b * b - 4.0f * a * c;
			if (discriminant < 0.0f)
				continue;

			// Quadratic formula:
			// (-b +- sqrt(discriminant)) / 2a

			// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
			float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
			if (closestT > 0.0f && closestT < hitDistance) {
				if (glm::length(closestT * ray.Direction + ray.Origin) >= border.Radius) {
					continue;
				}
				else {
					hitDistance = closestT;
					//hitDistance = closestT + ray.Traveled; // TODO: maybe need this
					closestSphere = (int)i;
				}
			}
		}

		if (closestSphere < 0) { // no sphere hit, do border check
			glm::vec3 originb = ray.Origin - border.Position;

			float a = glm::dot(centerFov, centerFov);
			float b = 2.0f * glm::dot(originb, centerFov);
			float c = glm::dot(originb, originb) - border.Radius * border.Radius;

			// Quadratic forumula discriminant:
			// b^2 - 4ac

			float discriminant = b * b - 4.0f * a * c;
			if (discriminant < 0.0f)
				continue;

			// Quadratic formula:
			// (-b +- sqrt(discriminant)) / 2a

			// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
			//closestB = (-b - glm::sqrt(discriminant)) / (2.0f * a);
			farthestB = (-b + glm::sqrt(discriminant)) / (2.0f * a);


			if (farthestB > 0.0f) {
				ray.Traveled = farthestB;
			}
		}
	}

	if (closestSphere < 0)
		if ((ray.Traveled < m_Settings.maxRayTravelDist) && (depth < m_Settings.maxRecursionDepth)) {
			//glm::vec3 originShift = -2.0f * (ray.Direction * farthestB + ray.Origin); // this isnt correct, needs to shift -border hit location
			
			glm::vec3 C = (centerFov * farthestB); // Vec from camera focal point to border hit
			// position based recursion
			//glm::vec3 C = (glm::normalize(ray.Origin) * (100.f - glm::length(ray.Origin)));
			//if (glm::dot(centerFov, C) <= 0.0f) {
			//	C = -C;
			//}
			glm::vec3 A = ray.Origin + C; // Vec from world origin to camera focal border hit
			ray.Origin = -(A + C);
			// Move recusrsion rays closer
			//ray.Origin = ray.Origin + ray.Direction * (lastB-closestB);
			//ray.Direction = glm::normalize(ray.Direction + centerFov);


			return TraceRay(ray, depth, centerFov, farthestB);
			//return TraceRay(ray, depth, centerFov, farthestB); // idea to pass in border hit, then shift ray origin to account for the delta from the sphere
		} else {
			return Miss(ray);
		}
	ray.Traveled = hitDistance;
	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex) {
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray) {
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}


//bool TriangleHit(Ray& ray, Triangle& triangle) {
//	bool hit = true;
//
//	// Check if the ray hits the plane
//	float nd = glm::dot(triangle.Normal, ray.Direction);
//	if (glm::abs(nd) < 0.0001f) {
//		hit = false;
//		return hit;
//	}
//	float nps = glm::dot(triangle.Normal, triangle.Verts[0] - ray.Origin);
//	float planeHitDist = nps / nd;
//	glm::vec3 planePoint = ray.Origin + ray.Direction * planeHitDist;
//
//	// Check that the plane-ray intersection is within the triangle
//	for (int e = 0; e < 3; e++) {
//		if (e < 2) {
//			glm::vec3 edge = triangle.Verts[e+1] - triangle.Verts[e];
//		} else {
//			glm::vec3 edge = triangle.Verts[0] - triangle.Verts[e];
//		}
//		glm::vec3 test = glm::cross(edge, planePoint - triangle.Verts[e]);
//		if (glm::dot(test, triangle.Normal) <= 0.0f) {
//			hit = false;
//			break; // if outside any edge break, crossing is outside triangle
//		}
//	}
//
//	return hit;
//}


//Renderer::glm::vec3 ComputeBRDF(const glm::vec3& albedo, float metallic, float roughness,
//                      const glm::vec3& normal, const glm::vec3& viewDir,
//                      const glm::vec3& lightDir, const glm::vec3& lightColor) {
//    glm::vec3 halfVector = glm::normalize(viewDir + lightDir);
//    float NdotL = std::max(glm::dot(normal, lightDir), 0.0f);
//    float NdotV = std::max(glm::dot(normal, viewDir), 0.0f);
//    float NdotH = std::max(glm::dot(normal, halfVector), 0.0f);
//    float HdotV = std::max(glm::dot(halfVector, viewDir), 0.0f);
//
//    // Fresnel (Schlick approximation)
//    glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic); // Dielectric F0 = 0.04, metal F0 = albedo
//    glm::vec3 fresnel = F0 + (1.0f - F0) * std::pow(1.0f - HdotV, 5.0f);
//
//    // Normal Distribution Function (GGX)
//    float alpha = roughness * roughness;
//    float alpha2 = alpha * alpha;
//    float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
//    float D = alpha2 / (M_PI * denom * denom);
//
//    // Geometry Term (Smith with GGX)
//    auto G_Smith = [](float NdotV, float roughness) {
//        float a = roughness * roughness;
//        float a2 = a * a;
//        float denom = NdotV * std::sqrt(a2 + (1.0f - a2) * NdotV * NdotV);
//        return 2.0f * NdotV / (NdotV + std::sqrt(a2 + (1.0f - a2) * NdotV * NdotV));
//    };
//    float G = G_Smith(NdotV, roughness) * G_Smith(NdotL, roughness);
//
//    // Specular term (Cook-Torrance)
//    glm::vec3 specular = (D * fresnel * G) / std::max(4.0f * NdotV * NdotL, 0.001f);
//
//    // Diffuse term (Lambertian, only for non-metals)
//    glm::vec3 diffuse = (1.0f - fresnel) * (1.0f - metallic) * albedo / M_PI;
//
//    // Combine diffuse and specular
//    return (diffuse + specular) * lightColor * NdotL;
//}