#include "MeshLoader.h"

#include "PLYLoader.h"
#include "Wankel/Renderer/Mesh.h"

#include <filesystem>
#include <stdexcept>

namespace Wankel {

std::unique_ptr<Mesh> MeshLoader::Load(const std::string& filepath) {
    std::filesystem::path path(filepath);
    std::string extension = path.extension().string();

    for (char& c : extension)
        c = (char)tolower(c);

    if (extension == ".ply") {
        auto meshData = PLYLoader::Load(filepath);
        std::vector<Vertex> vertices;
        constexpr size_t stride = 7;
        size_t vertexCount = meshData.Vertices.size() / stride;
        vertices.reserve(vertexCount);

        for (size_t i = 0; i < vertexCount; i++) {
            size_t base = i * stride;

            Vertex v;

            v.Position = {meshData.Vertices[base + 0], meshData.Vertices[base + 1], meshData.Vertices[base + 2]};

            v.Color = {meshData.Vertices[base + 3], meshData.Vertices[base + 4], meshData.Vertices[base + 5],
                       meshData.Vertices[base + 6]};

            vertices.push_back(v);
        }

        return std::make_unique<Mesh>(vertices, meshData.Indices);
    }

    throw std::runtime_error("Unsupported mesh format: " + extension);
}

} // namespace Wankel
