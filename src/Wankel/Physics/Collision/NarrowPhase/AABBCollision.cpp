#include "AABBCollision.h"

namespace Wankel {

CollisionManifold AABBvsAABB(const AABB& a, const AABB& b)
{
    CollisionManifold m;

    if (!a.Intersects(b))
        return m;

    m.Colliding = true;

    // compute overlap on each axis
    float dx1 = b.Max.x - a.Min.x;
    float dx2 = a.Max.x - b.Min.x;
    float dy1 = b.Max.y - a.Min.y;
    float dy2 = a.Max.y - b.Min.y;
    float dz1 = b.Max.z - a.Min.z;
    float dz2 = a.Max.z - b.Min.z;

    float px = (dx1 < dx2) ? dx1 : -dx2;
    float py = (dy1 < dy2) ? dy1 : -dy2;
    float pz = (dz1 < dz2) ? dz1 : -dz2;

    // choose smallest penetration axis
    float absX = abs(px);
    float absY = abs(py);
    float absZ = abs(pz);

    if (absX < absY && absX < absZ) {
        m.Penetration = absX;
        m.Normal = { (px > 0) ? 1.0f : -1.0f, 0, 0 };
    }
    else if (absY < absZ) {
        m.Penetration = absY;
        m.Normal = { 0, (py > 0) ? 1.0f : -1.0f, 0 };
    }
    else {
        m.Penetration = absZ;
        m.Normal = { 0, 0, (pz > 0) ? 1.0f : -1.0f };
    }

    return m;
}

}
