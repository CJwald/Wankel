#pragma once

#include "../CollisionManifold.h"
#include "Sphere.h"

namespace Wankel {

CollisionManifold SpherevsSphere(
    const Sphere& a,
    const Sphere& b);

}
