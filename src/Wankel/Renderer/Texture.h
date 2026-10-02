#pragma once

#include "Wankel/Core/Base.h"

#include <cstdint>
#include <string>

namespace Wankel {

// R8 = single channel (font atlases, masks); RGBA8 = color + alpha (particle sprites, UI images).
enum class TextureFormat : uint8_t { R8, RGBA8 };

// Minimal GL 2D texture wrapper - linear filtering, clamp-to-edge.
class Texture {
public:
    // `pixels` is width*height texels in `format`, rows tightly packed. mipmaps adds a mip chain with
    // trilinear minification (worth it for anything drawn small/far away, e.g. particles).
    Texture(const uint8_t* pixels, uint32_t width, uint32_t height, TextureFormat format = TextureFormat::R8,
            bool mipmaps = false);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // RGBA8 + mipmaps from an image file (png/jpg/tga/...), flipped so UV (0,0) is the image's bottom-left.
    // Null (with an error logged) if the file can't be read.
    static Ref<Texture> LoadFromFile(const std::string& path);

    void Bind(uint32_t slot = 0) const;

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetID() const { return m_ID; } // for ImGui::Image previews

private:
    unsigned int m_ID = 0;
    uint32_t m_Width, m_Height;
};

} // namespace Wankel
