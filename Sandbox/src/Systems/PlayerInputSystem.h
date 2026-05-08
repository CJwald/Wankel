#pragma once

#include <Wankel/ECS/Scene.h>

namespace Wankel {

class PlayerInputSystem {
public:
    void Update(Scene& scene, float dt, bool gameFocused);
};

}
