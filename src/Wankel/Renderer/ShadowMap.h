#pragma once

#include <cstdint>

namespace Wankel {

// Depth-only render target for shadow-style passes (see Renderer::BeginShadowPass): a square 32-bit float
// depth texture with hardware depth comparison on, so shaders sample it as a sampler2DShadow (free 2x2 PCF).
// Outside the map it reads as fully lit (clamp-to-border depth 1).
class ShadowMap {
public:
    explicit ShadowMap(uint32_t resolution);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Reallocates the depth texture; a no-op when unchanged.
    void Resize(uint32_t resolution);

    uint32_t GetResolution() const { return m_Resolution; }
    uint32_t GetTextureID() const { return m_DepthTexture; }
    uint32_t GetFramebufferID() const { return m_Framebuffer; }

private:
    void Allocate();

    uint32_t m_Resolution = 0;
    uint32_t m_Framebuffer = 0;
    uint32_t m_DepthTexture = 0;
};

} // namespace Wankel
