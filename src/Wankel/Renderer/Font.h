#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

#include "Wankel/Core/Base.h"

namespace Wankel {

class Texture;

struct GlyphQuad {
    glm::vec2 Min, Max;     // screen-space corners (pixels, Y-down)
    glm::vec2 UVMin, UVMax; // font atlas texture coordinates
};

// A single baked bitmap font: one .ttf, one pixel size, ASCII 32-127 baked
// into a texture atlas via stb_truetype at load time. Deliberately basic
// (no SDF, no dynamic glyph ranges, no Unicode) - just enough for HUD/UI
// text; see Documents/TODO.md for what a fuller text/UI system would add.
class Font {
public:
    static Ref<Font> Load(const std::string& ttfPath, float pixelHeight = 48.0f);

    // Appends the quads needed to draw `text` starting at `penStart`
    // (screen-space pixels, Y-down, baseline-relative - same convention
    // stb_truetype itself uses), returning the pen's final X position so
    // callers can measure/right-align text.
    float BuildQuads(const std::string& text, glm::vec2 penStart, std::vector<GlyphQuad>& outQuads) const;
    float MeasureWidth(const std::string& text) const;

    const Ref<Texture>& GetAtlasTexture() const { return m_AtlasTexture; }

private:
    Ref<Texture> m_AtlasTexture;
    std::vector<uint8_t> m_BakedChars; // opaque stbtt_bakedchar[] storage - kept out of the public header
    int m_AtlasWidth = 0;
    int m_AtlasHeight = 0;
};

} // namespace Wankel
