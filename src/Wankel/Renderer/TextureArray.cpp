#include "wkpch.h"
#include "TextureArray.h"

#include <glad/gl.h>

#include <algorithm>
#include <cmath>

namespace Wankel {

namespace {

GLenum ExternalFormat(TextureArrayFormat format) {
    switch (format) {
        case TextureArrayFormat::R8:
            return GL_RED;
        case TextureArrayFormat::RG8:
            return GL_RG;
        case TextureArrayFormat::RGBA8:
            return GL_RGBA;
        case TextureArrayFormat::RGB8:
        case TextureArrayFormat::SRGB8:
        default:
            return GL_RGB;
    }
}

GLenum InternalFormat(TextureArrayFormat format) {
    switch (format) {
        case TextureArrayFormat::R8:
            return GL_R8;
        case TextureArrayFormat::RG8:
            return GL_RG8;
        case TextureArrayFormat::RGBA8:
            return GL_RGBA8;
        case TextureArrayFormat::SRGB8:
            return GL_SRGB8;
        case TextureArrayFormat::RGB8:
        default:
            return GL_RGB8;
    }
}

} // namespace

uint32_t TextureArrayChannels(TextureArrayFormat format) {
    switch (format) {
        case TextureArrayFormat::R8:
            return 1;
        case TextureArrayFormat::RG8:
            return 2;
        case TextureArrayFormat::RGBA8:
            return 4;
        case TextureArrayFormat::RGB8:
        case TextureArrayFormat::SRGB8:
        default:
            return 3;
    }
}

TextureArray::TextureArray(uint32_t size, uint32_t layers, TextureArrayFormat format)
    : m_Size(size), m_Layers(layers), m_Format(format) {
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ID);

    GLsizei levels = 1 + (GLsizei)std::floor(std::log2((float)std::max(size, 1u)));
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, InternalFormat(format), (GLsizei)size, (GLsizei)size, (GLsizei)layers);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

TextureArray::~TextureArray() {
    glDeleteTextures(1, &m_ID);
}

void TextureArray::SetLayer(uint32_t layer, const uint8_t* pixels) {
    if (layer >= m_Layers)
        return;

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ID);

    // GL_UNPACK_ALIGNMENT is global context state - RGB8/R8 rows needn't be 4-byte aligned, so restore after.
    GLint previousAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)layer, (GLsizei)m_Size, (GLsizei)m_Size, 1,
                    ExternalFormat(m_Format), GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
}

void TextureArray::GenerateMips() {
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ID);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void TextureArray::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ID);
}

} // namespace Wankel
