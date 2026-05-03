#pragma once

#include "../Collision/BroadPhase/SpatialHashGrid.h"

namespace Wankel {

class Scene;

class PhysicsSystem {
public:
    void Update(Scene& scene, float dt);

private:
    SpatialHashGrid m_Grid{1.0f}; // cell size ~ cube size
};

}
