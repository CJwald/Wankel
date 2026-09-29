#pragma once

#include <cstdint>

namespace Wankel {

enum class TextureArrayFormat : uint8_t { R8, RGB8 };

// GL_TEXTURE_2D_ARRAY wrapper: square layers, repeat wrap, trilinear mipmapped filtering.
class TextureArray {
public:
    TextureArray(uint32_t size, uint32_t layers, TextureArrayFormat format);
    ~TextureArray();

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    // pixels must be size*size texels in this array's format, tightly packed.
    void SetLayer(uint32_t layer, const uint8_t* pixels);
    void GenerateMips();
    void Bind(uint32_t slot) const;

    uint32_t GetSize() const { return m_Size; }
    uint32_t GetLayerCount() const { return m_Layers; }
    TextureArrayFormat GetFormat() const { return m_Format; }

private:
    unsigned int m_ID = 0;
    uint32_t m_Size, m_Layers;
    TextureArrayFormat m_Format;
};

} // namespace Wankel
