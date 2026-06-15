#pragma once

namespace Wankel {

	class Scene;
	
	class TransformSystem {
	public:
		void Update(Scene& scene, float dt);
	
	private:
		void UpdateTransforms(Scene& scene, float dt);
		void UpdateFinalTransforms(Scene& scene);
	
		float m_dPosThreshold = 50.0f;
	};
}
