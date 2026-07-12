#include "wkpch.h"
#include "Font.h"

#include "Texture.h"

#include <fstream>
#include <stdexcept>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace Wankel {

namespace {

constexpr int kFirstChar = 32;
constexpr int kNumChars = 96; // ASCII 32..127 - covers standard printable text
constexpr int kAtlasSize = 512; // fixed square atlas; comfortably fits one baked ASCII set

} // namespace

Ref<Font> Font::Load(const std::string& ttfPath, float pixelHeight) {
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("Font: failed to open '" + ttfPath + "'");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> fontData((size_t)size);
    if (!file.read(reinterpret_cast<char*>(fontData.data()), size))
        throw std::runtime_error("Font: failed to read '" + ttfPath + "'");

    std::vector<uint8_t> atlasPixels((size_t)(kAtlasSize * kAtlasSize));
    std::vector<uint8_t> bakedChars(sizeof(stbtt_bakedchar) * kNumChars);

    int result = stbtt_BakeFontBitmap(fontData.data(), 0, pixelHeight, atlasPixels.data(), kAtlasSize, kAtlasSize,
                                       kFirstChar, kNumChars, reinterpret_cast<stbtt_bakedchar*>(bakedChars.data()));

    if (result <= 0)
        throw std::runtime_error("Font: '" + ttfPath + "' didn't fit in the " + std::to_string(kAtlasSize) + "x" +
                                  std::to_string(kAtlasSize) + " atlas at pixel height " +
                                  std::to_string(pixelHeight));

    auto font = CreateRef<Font>();
    font->m_AtlasTexture = CreateRef<Texture>(atlasPixels.data(), (uint32_t)kAtlasSize, (uint32_t)kAtlasSize);
    font->m_BakedChars = std::move(bakedChars);
    font->m_AtlasWidth = kAtlasSize;
    font->m_AtlasHeight = kAtlasSize;

    WK_CORE_INFO("Font: loaded '{0}' ({1}px, atlas {2}x{2}, {3} rows used)", ttfPath, pixelHeight, kAtlasSize,
                 result);

    return font;
}

float Font::BuildQuads(const std::string& text, glm::vec2 penStart, std::vector<GlyphQuad>& outQuads) const {
    const auto* baked = reinterpret_cast<const stbtt_bakedchar*>(m_BakedChars.data());

    float x = penStart.x;
    float y = penStart.y;

    for (char c : text) {
        if (c < kFirstChar || c >= kFirstChar + kNumChars)
            continue;

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(baked, m_AtlasWidth, m_AtlasHeight, c - kFirstChar, &x, &y, &q, 1);

        GlyphQuad quad;
        quad.Min = {q.x0, q.y0};
        quad.Max = {q.x1, q.y1};
        quad.UVMin = {q.s0, q.t0};
        quad.UVMax = {q.s1, q.t1};
        outQuads.push_back(quad);
    }

    return x;
}

float Font::MeasureWidth(const std::string& text) const {
    std::vector<GlyphQuad> scratch;
    return BuildQuads(text, {0.0f, 0.0f}, scratch) - 0.0f;
}

} // namespace Wankel
