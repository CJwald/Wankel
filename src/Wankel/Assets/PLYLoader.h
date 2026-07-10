#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "Wankel/Renderer/Mesh.h"

namespace Wankel {

class PLYLoader {
public:
    // Loads an ASCII PLY file (x y z r g b a per vertex) into the engine's
    // canonical Vertex/index format. Throws std::runtime_error on failure
    // (missing file, non-ASCII format, malformed header).
    static void Load(const std::string& path, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};

} // namespace Wankel
