#include "wkpch.h"
#include "PLYLoader.h"
#include "MeshUtils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Wankel {

namespace {

void Trim(std::string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

} // namespace

void PLYLoader::Load(const std::string& path, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("PLYLoader: failed to open file: " + path);

    outVertices.clear();
    outIndices.clear();

    std::string line;
    uint32_t vertexCount = 0;
    uint32_t faceCount = 0;
    bool sawFormatLine = false;

    // ================= HEADER =================
    WK_CORE_TRACE("PLYLoader: parsing header for '{0}'", path);

    while (std::getline(file, line)) {
        Trim(line);

        if (line.rfind("format", 0) == 0) {
            sawFormatLine = true;

            // This parser only supports the ASCII variant - binary_little_endian
            // / binary_big_endian PLY would silently parse into garbage if we
            // read it as text, so fail loudly instead.
            if (line.find("ascii") == std::string::npos)
                throw std::runtime_error("PLYLoader: '" + path +
                                          "' is not an ASCII PLY file (only ASCII is supported): " + line);
        } else if (line.find("element vertex") != std::string::npos) {
            std::stringstream ss(line);
            std::string tmp;
            ss >> tmp >> tmp >> vertexCount;
        } else if (line.find("element face") != std::string::npos) {
            std::stringstream ss(line);
            std::string tmp;
            ss >> tmp >> tmp >> faceCount;
        } else if (line == "end_header") {
            break;
        }
    }

    if (!sawFormatLine)
        throw std::runtime_error("PLYLoader: '" + path + "' has no 'format' header line");

    // ================= VERTICES =================
    outVertices.reserve(vertexCount);

    for (uint32_t i = 0; i < vertexCount; ++i) {
        std::getline(file, line);
        std::stringstream ss(line);

        float x, y, z;
        int r, g, b, a;

        ss >> x >> y >> z;

        // Blender -> Engine axis conversion. This engine's ASCII PLY assets
        // are exported straight from Blender's (Z-up) native space with no
        // axis conversion applied by Blender's PLY exporter, unlike glTF
        // (see GltfLoader, which needs no equivalent remap).
        float ex = -y;
        float ey = z;
        float ez = -x;

        ss >> r >> g >> b >> a; // uchar properties read as int

        Vertex v;
        v.Position = {ex, ey, ez};
        v.Color = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
        // v.Normal left at its default; PLY carries no source normal data,
        // so real normals get computed below from triangle winding.

        outVertices.push_back(v);
    }

    // ================= FACES =================
    uint32_t facesRead = 0;

    while (std::getline(file, line)) {
        Trim(line);

        if (line.empty())
            continue;

        std::stringstream ss(line);

        int n = 0;
        if (!(ss >> n))
            continue;

        if (n < 3)
            continue;

        std::vector<uint32_t> idx(n);
        bool valid = true;

        for (int i = 0; i < n; ++i) {
            if (!(ss >> idx[i])) {
                WK_CORE_WARNING("PLYLoader: face parse failed in '{0}': {1}", path, line);
                valid = false;
                break;
            }

            if (idx[i] >= vertexCount) {
                WK_CORE_WARNING("PLYLoader: face index {0} out of range (vertex count {1}) in '{2}'", idx[i],
                                 vertexCount, path);
                valid = false;
                break;
            }
        }

        if (!valid)
            continue;

        // Triangulate (fan method).
        for (int i = 1; i + 1 < n; ++i) {
            outIndices.push_back(idx[0]);
            outIndices.push_back(idx[i]);
            outIndices.push_back(idx[i + 1]);
        }

        facesRead++;
    }

    WK_CORE_INFO("PLYLoader: loaded '{0}' ({1} vertices, {2} faces, {3} indices)", path, outVertices.size(),
                 facesRead, outIndices.size());

    ComputeSmoothNormals(outVertices, outIndices);
}

} // namespace Wankel
