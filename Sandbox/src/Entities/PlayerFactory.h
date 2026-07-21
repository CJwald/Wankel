#pragma once

#include <Wankel/ECS/Entity.h>

namespace Wankel {
class Scene;
}

// Builds the player rig: root entity (PlayerController/collider/physics),
// head, 4 legs, gun, and camera, each parented to the root.
class PlayerFactory {
public:
    static Wankel::Entity Create(Wankel::Scene& scene);
};
