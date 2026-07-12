#pragma once

#include <cstdint>

namespace Wankel {

// Minimal GL texture wrapper. Single-channel (R8) only for now - just
// enough to back a font atlas; a general RGBA path can be added once the
// engine has a real material/texture pipeline (see Documents/TODO.md).
class Texture {
public:
    Texture(const uint8_t* pixels, uint32_t width, uint32_t height);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Bind(uint32_t slot = 0) const;

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    unsigned int m_ID = 0;
    uint32_t m_Width, m_Height;
};

} // namespace Wankel
