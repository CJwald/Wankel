#include "wkpch.h"
#include "Texture.h"

#include <glad/gl.h>

namespace Wankel {

Texture::Texture(const uint8_t* pixels, uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Atlas bitmaps (e.g. stb_truetype's baked font atlas) are single-channel
    // (coverage/alpha only) - GL_RED keeps this a 1-byte-per-pixel upload
    // instead of wasting 4x the memory on an RGBA texture.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei)width, (GLsizei)height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_ID);
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

} // namespace Wankel
