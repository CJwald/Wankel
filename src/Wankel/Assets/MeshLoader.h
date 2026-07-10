#pragma once

#include <memory>
#include <string>

namespace Wankel {

class Mesh;

// Dispatches to the appropriate importer by file extension (.ply, .gltf,
// .glb), all of which feed the same canonical Vertex/index format.
class MeshLoader {
public:
    static std::unique_ptr<Mesh> Load(const std::string& filepath);
};

} // namespace Wankel
