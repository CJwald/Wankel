#pragma once
#include <vector>
#include <string>
#include <glad/gl.h>

#include <Wankel/Core/Log.h>

namespace Wankel {

struct BufferElement {
    std::string Name;
    GLenum Type;
    uint32_t Count;
    uint32_t Offset;
    bool Normalized;

    BufferElement(std::string name, GLenum type, uint32_t count, uint32_t offset, bool normalized)
        : Name(std::move(name)), Type(type), Count(count), Offset(offset), Normalized(normalized) {}

    static uint32_t GetSizeOfType(GLenum type) {
        switch (type) {
            case GL_FLOAT:
                return 4;
            case GL_UNSIGNED_INT:
                return 4;
            case GL_UNSIGNED_BYTE:
                return 1;
            case GL_UNSIGNED_SHORT:
                return 2;
            default:
                WK_CORE_ERROR("VertexBufferLayout::GetSizeOfType - unhandled GLenum {0}", (uint32_t)type);
                return 0;
        }
    }
};

class VertexBufferLayout {
public:
    VertexBufferLayout() : m_Stride(0) {}

    void PushFloat(uint32_t count, const std::string& name, bool normalized = false) {
        m_Elements.emplace_back(name, GL_FLOAT, count, m_Stride, normalized);
        m_Stride += count * BufferElement::GetSizeOfType(GL_FLOAT);
    }

    void PushUInt(uint32_t count, const std::string& name) {
        m_Elements.emplace_back(name, GL_UNSIGNED_INT, count, m_Stride, false);
        m_Stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_INT);
    }

    void PushUChar(uint32_t count, const std::string& name, bool normalized = true) {
        m_Elements.emplace_back(name, GL_UNSIGNED_BYTE, count, m_Stride, normalized);
        m_Stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_BYTE);
    }

    void PushUShort(uint32_t count, const std::string& name, bool normalized = true) {
        m_Elements.emplace_back(name, GL_UNSIGNED_SHORT, count, m_Stride, normalized);
        m_Stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_SHORT);
    }

    // GL_INT_2_10_10_10_REV: 4 logical components (x,y,z,w) packed into one 32-bit value -
    // Count=4 is what glVertexAttribPointer requires for this type; the actual stride
    // contribution is the packed value's own size (4 bytes), not 4 components x 4 bytes, so this
    // doesn't go through the count * GetSizeOfType(type) formula the other Push* methods use.
    void PushPackedNormal(const std::string& name) {
        m_Elements.emplace_back(name, GL_INT_2_10_10_10_REV, 4, m_Stride, true);
        m_Stride += 4;
    }

    // Reserves `bytes` of stride with no corresponding attribute - for forcing a later Push*'s offset
    // onto a natural alignment boundary in a #pragma pack(1) vertex struct (see QuantizedVertex in
    // Mesh.cpp), where the compiler's usual alignment padding has been deliberately removed to hit an
    // exact manually-tracked stride and needs to be re-added by hand where it actually matters.
    void PushPadding(uint32_t bytes) { m_Stride += bytes; }

    inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }
    inline uint32_t GetStride() const { return m_Stride; }

private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride;
};

} // namespace Wankel
