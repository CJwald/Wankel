#include "wkpch.h"
#include "MeshLoader.h"

#include "PLYLoader.h"
#include "GltfLoader.h"
#include "Wankel/Renderer/Mesh.h"

#include <filesystem>
#include <stdexcept>

namespace Wankel {

std::unique_ptr<Mesh> MeshLoader::Load(const std::string& filepath) {
    std::filesystem::path path(filepath);
    std::string extension = path.extension().string();

    for (char& c : extension)
        c = (char)tolower(c);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    if (extension == ".ply") {
        PLYLoader::Load(filepath, vertices, indices);
    } else if (extension == ".gltf" || extension == ".glb") {
        GltfLoader::Load(filepath, vertices, indices);
    } else {
        throw std::runtime_error("MeshLoader: unsupported mesh format: " + extension);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace Wankel
