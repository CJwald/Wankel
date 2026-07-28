#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include "VertexBufferLayout.h"

namespace {

// Packs a normalized vec3 into GL_INT_2_10_10_10_REV layout (X:bits0-9, Y:bits10-19,
// Z:bits20-29, W:bits30-31, each a signed 10-bit component) - GL unpacks this to a normalized
// vec4 in the shader for free, no decode ALU cost, at 4 bytes instead of a plain vec3's 12.
uint32_t PackNormal(const glm::vec3& n) {
    glm::vec3 c = glm::clamp(n, -1.0f, 1.0f);
    auto packComponent = [](float v) -> uint32_t {
        return (uint32_t)(int32_t)std::lround(v * 511.0f) & 0x3FF; // 10-bit signed, masked
    };
    return packComponent(c.x) | (packComponent(c.y) << 10) | (packComponent(c.z) << 20);
}

// GPU-side layout: identical to Wankel::Vertex except Normal is packed - built once per Mesh
// upload so every mesh producer (glTF/PLY loaders, VoxelMesher, CreateMirrored) keeps working
// with plain vec3 normals; only the GPU buffer itself is compressed.
struct PackedVertex {
    glm::vec3 Position;
    glm::vec4 Color;
    uint32_t PackedNormal;
};

// Same as PackedVertex but with Position also quantized (see Mesh's quantizing ctor). Packed to
// 1-byte alignment so sizeof() is exactly 6+16+4=26 with no compiler-inserted padding between the
// 2-byte Position array and the 4-byte-aligned fields after it - VertexBufferLayout's manually
// tracked stride below must match this struct's true byte layout exactly, or the GPU reads every
// attribute at the wrong offset.
#pragma pack(push, 1)
struct QuantizedVertex {
    uint16_t Position[3];
    glm::vec4 Color;
    uint32_t PackedNormal;
};
#pragma pack(pop)
static_assert(sizeof(QuantizedVertex) == 26, "QuantizedVertex must be tightly packed - GPU stride depends on this");

uint16_t QuantizeComponent(float value, float min, float extent) {
    float t = extent > 1e-8f ? glm::clamp((value - min) / extent, 0.0f, 1.0f) : 0.0f;
    return (uint16_t)std::lround(t * 65535.0f);
}

// Shared by the quantizing constructor and UpdateData, so a mesh's GPU buffers can be respecified
// with fresh data (see UpdateData) without duplicating this packing logic.
std::vector<QuantizedVertex> BuildQuantizedVertices(const std::vector<Wankel::Vertex>& vertices, const glm::vec3& min,
                                                     const glm::vec3& extent) {
    std::vector<QuantizedVertex> packed;
    packed.reserve(vertices.size());
    for (const Wankel::Vertex& v : vertices) {
        QuantizedVertex qv;
        qv.Position[0] = QuantizeComponent(v.Position.x, min.x, extent.x);
        qv.Position[1] = QuantizeComponent(v.Position.y, min.y, extent.y);
        qv.Position[2] = QuantizeComponent(v.Position.z, min.z, extent.z);
        qv.Color = v.Color;
        qv.PackedNormal = PackNormal(v.Normal);
        packed.push_back(qv);
    }
    return packed;
}

} // namespace

namespace Wankel {

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_Vertices(vertices), m_Indices(indices), m_IndexCount((uint32_t)indices.size()) {
    m_VertexArray = std::make_unique<VertexArray>();
    m_VertexArray->Bind();

    std::vector<PackedVertex> packed;
    packed.reserve(m_Vertices.size());
    for (const Vertex& v : m_Vertices)
        packed.push_back({v.Position, v.Color, PackNormal(v.Normal)});

    m_VertexBuffer = std::make_unique<VertexBuffer>(packed.data(), (uint32_t)(packed.size() * sizeof(PackedVertex)));

    m_IndexBuffer = std::make_unique<IndexBuffer>(m_Indices.data(), m_IndexCount);

    VertexBufferLayout layout;
    layout.PushFloat(3, "a_Position");
    layout.PushFloat(4, "a_Color");
    layout.PushPackedNormal("a_Normal");

    m_VertexBuffer->SetLayout(layout);

    m_VertexBuffer->Bind();
    m_VertexArray->AddVertexBuffer(*m_VertexBuffer);

    m_VertexArray->SetIndexBuffer(*m_IndexBuffer);
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::vec3& quantizeMin,
          const glm::vec3& quantizeMax)
    : m_Vertices(vertices), m_Indices(indices), m_IndexCount((uint32_t)indices.size()), m_QuantizeMin(quantizeMin),
      m_QuantizeExtent(quantizeMax - quantizeMin) {
    m_VertexArray = std::make_unique<VertexArray>();
    m_VertexArray->Bind();

    std::vector<QuantizedVertex> packed = BuildQuantizedVertices(m_Vertices, m_QuantizeMin, m_QuantizeExtent);

    m_VertexBuffer =
        std::make_unique<VertexBuffer>(packed.data(), (uint32_t)(packed.size() * sizeof(QuantizedVertex)));

    m_IndexBuffer = std::make_unique<IndexBuffer>(m_Indices.data(), m_IndexCount);

    VertexBufferLayout layout;
    layout.PushUShort(3, "a_Position");
    layout.PushFloat(4, "a_Color");
    layout.PushPackedNormal("a_Normal");

    m_VertexBuffer->SetLayout(layout);

    m_VertexBuffer->Bind();
    m_VertexArray->AddVertexBuffer(*m_VertexBuffer);

    m_VertexArray->SetIndexBuffer(*m_IndexBuffer);
}

void Mesh::UpdateData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                      const glm::vec3& quantizeMin, const glm::vec3& quantizeMax) {
    m_Vertices = vertices;
    m_Indices = indices;
    m_IndexCount = (uint32_t)indices.size();
    m_QuantizeMin = quantizeMin;
    m_QuantizeExtent = quantizeMax - quantizeMin;

    // No VAO/attribute rework needed - glVertexAttribPointer bound to these buffers' GL object IDs
    // at construction time stays valid across a later respecify, as long as the layout shape
    // (always true here: this ctor's quantized layout never changes shape between calls) doesn't.
    std::vector<QuantizedVertex> packed = BuildQuantizedVertices(m_Vertices, m_QuantizeMin, m_QuantizeExtent);
    m_VertexBuffer->SetData(packed.data(), (uint32_t)(packed.size() * sizeof(QuantizedVertex)));
    m_IndexBuffer->SetData(m_Indices.data(), m_IndexCount);
}

Mesh::~Mesh() {}

Mesh::Mesh(Mesh&&) noexcept = default;
Mesh& Mesh::operator=(Mesh&&) noexcept = default;

void Mesh::Bind() const {
    m_VertexArray->Bind();
}

uint32_t Mesh::GetIndexCount() const {
    return m_IndexBuffer->GetCount();
}

std::unique_ptr<Mesh> Mesh::CreateMirrored(bool mirrorX, bool mirrorY, bool mirrorZ) const {
    auto vertices = m_Vertices;
    auto indices = m_Indices;

    for (auto& v : vertices) {
        if (mirrorX) {
            v.Position.x *= -1.0f;
            v.Normal.x *= -1.0f;
        }
        if (mirrorY) {
            v.Position.y *= -1.0f;
            v.Normal.y *= -1.0f;
        }
        if (mirrorZ) {
            v.Position.z *= -1.0f;
            v.Normal.z *= -1.0f;
        }
    }

    // Count number of mirrored axes
    int mirrorCount = 0;
    if (mirrorX)
        mirrorCount++;
    if (mirrorY)
        mirrorCount++;
    if (mirrorZ)
        mirrorCount++;

    // Odd number of reflections flips handedness
    if (mirrorCount % 2 == 1) {
        for (size_t i = 0; i < indices.size(); i += 3) {
            std::swap(indices[i + 1], indices[i + 2]);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace Wankel
