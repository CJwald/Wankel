#include "wkpch.h"
#include "Texture.h"

#include <glad/gl.h>
#include <stb_image.h> // implementation compiled in TerrainMaterial.cpp

namespace Wankel {

Texture::Texture(const uint8_t* pixels, uint32_t width, uint32_t height, TextureFormat format, bool mipmaps)
    : m_Width(width), m_Height(height) {
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Single-channel atlases (e.g. stb_truetype's baked font atlas) stay GL_RED - 1 byte per texel.
    //
    // GL_UNPACK_ALIGNMENT is global context state, not scoped to this
    // texture/bind, so it has to be saved and restored - left at 1, it
    // would silently affect every glTexImage2D/glTexSubImage2D call made
    // anywhere else in the process afterward.
    GLint previousAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (format == TextureFormat::RGBA8)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei)width, (GLsizei)height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    if (mipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_ID);
}

Ref<Texture> Texture::LoadFromFile(const std::string& path) {
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(0);
    if (!pixels) {
        WK_CORE_ERROR("Texture - failed to load '{0}': {1}", path, stbi_failure_reason());
        return nullptr;
    }

    Ref<Texture> texture = CreateRef<Texture>(pixels, (uint32_t)width, (uint32_t)height, TextureFormat::RGBA8, true);
    stbi_image_free(pixels);
    return texture;
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

} // namespace Wankel
