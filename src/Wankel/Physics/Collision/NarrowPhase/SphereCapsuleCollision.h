#pragma once

#include "../CollisionManifold.h"
#include "Sphere.h"
#include "Capsule.h"

namespace Wankel {

CollisionManifold SpherevsCapsule(const Sphere& sphere, const Capsule& capsule);

}
