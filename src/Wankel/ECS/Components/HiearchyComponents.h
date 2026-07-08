#pragma once
#include "Wankel/ECS/Entity.h"


namespace Wankel {

struct Parent {
    Entity Parent;

    bool InheritPosition = true;
    bool InheritRotation = true;
    bool InheritScale = true;

    bool InheritLinearVelocity = true;
    bool InheritAngularVelocity = true;
};


struct Children {
    std::vector<Entity> Children;
};

} // namespace Wankel
