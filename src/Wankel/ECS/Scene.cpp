#include "wkpch.h"
#include "Scene.h"
#include "Components.h"
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/Renderer/Camera.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

    void Scene::OnUpdate(float dt, Camera& camera) {

        m_PhysicsSystem.Update(*this, dt);
		UpdateTransforms(dt);
		UpdateProceduralAnimation(dt);
		UpdateFinalTransforms();

	}
}
