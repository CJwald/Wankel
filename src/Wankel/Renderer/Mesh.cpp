#include "wkpch.h"
#include "Mesh.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "IndexBuffer.h"
#include "VertexBufferLayout.h"
#include "QuantizedVertex.h"

namespace {

// GPU-side layout: identical to Wankel::Vertex except Normal is packed - built once per Mesh
// upload so every mesh producer (glTF/PLY loaders, VoxelMesher, CreateMirrored) keeps working
// with plain vec3 normals; only the GPU buffer itself is compressed.
struct PackedVertex {
    glm::vec3 Position;
    glm::vec4 Color;
    uint32_t PackedNormal;
};

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

    m_VertexBuffer = std::make_unique<VertexBuffer>(packed.data(), (uint32_t)(packed.size() * sizeof(QuantizedVertex)));

    m_IndexBuffer = std::make_unique<IndexBuffer>(m_Indices.data(), m_IndexCount);

    VertexBufferLayout layout;
    layout.PushUShort(3, "a_Position");
    layout.PushPadding(2); // see QuantizedVertex's own comment - keeps Color/PackedNormal 4-byte aligned
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
