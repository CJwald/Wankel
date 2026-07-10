#pragma once

#include <vector>
#include <cstdint>

namespace Wankel {

struct Vertex;

// Computes smooth (area-weighted, averaged) per-vertex normals from triangle
// winding. Used as a fallback for source formats that don't carry real
// normal data (e.g. this engine's ASCII PLY assets), or a glTF primitive
// that's missing its NORMAL attribute.
void ComputeSmoothNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

} // namespace Wankel
