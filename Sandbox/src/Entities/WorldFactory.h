#pragma once

namespace Wankel {
class Scene;
}

// Builds the world: 20 randomly-placed/oriented cubes and the "World"
// ground entity.
class WorldFactory {
public:
    static void Create(Wankel::Scene& scene);
};
