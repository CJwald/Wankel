#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "Wankel/Renderer/Mesh.h"
#include "Wankel/Renderer/Renderer.h" // Material

namespace Wankel {

class GltfLoader {
public:
    // Loads a .gltf or .glb file into the engine's canonical Vertex/index
    // format, walking every node in the default scene (or every node in the
    // file if no scene is defined). Throws std::runtime_error on failure.
    // outMaterial is set from the first primitive's material found while
    // walking the file (Albedo/Metallic/Roughness/Emissive factors only -
    // no textures yet); left at Material{}'s own default if the file has no
    // material at all.
    static void Load(const std::string& path, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices,
                     Material& outMaterial);
};

} // namespace Wankel
