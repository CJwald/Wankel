#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "Wankel/Renderer/Mesh.h"

namespace Wankel {

class GltfLoader {
public:
    // Loads a .gltf or .glb file into the engine's canonical Vertex/index
    // format, walking every node in the default scene (or every node in the
    // file if no scene is defined). Throws std::runtime_error on failure.
    static void Load(const std::string& path, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};

} // namespace Wankel
