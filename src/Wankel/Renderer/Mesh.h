#pragma once

#include <memory>

namespace Wankel {

class VertexArray;
class VertexBuffer;
class VertexBufferLayout;

class Mesh {
public:
    Mesh(const void* vertices, uint32_t size, uint32_t vertexCount);
    ~Mesh();

    void Bind() const;

    uint32_t GetVertexCount() const { return m_VertexCount; }

private:
    std::unique_ptr<VertexArray> m_VertexArray;
    std::unique_ptr<VertexBuffer> m_VertexBuffer;
    uint32_t m_VertexCount = 0;
};

}
