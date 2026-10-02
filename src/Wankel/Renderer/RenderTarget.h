#pragma once

#include <cstdint>

namespace Wankel {

// Offscreen RGBA8 + D24S8 framebuffer (D24S8 so Renderer::CaptureSceneDepth can blit from it), e.g. editor previews.
class RenderTarget {
public:
    RenderTarget(uint32_t width, uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // Reallocates the attachments; a no-op when the size is unchanged.
    void Resize(uint32_t width, uint32_t height);

    // Binds it with a full viewport; End() restores the previous framebuffer and viewport. Not nestable.
    void Begin();
    void End();

    uint32_t GetColorTexture() const { return m_Color; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void Allocate();

    uint32_t m_Width = 0, m_Height = 0;
    uint32_t m_Framebuffer = 0;
    uint32_t m_Color = 0;
    uint32_t m_Depth = 0;

    int m_PreviousFramebuffer = 0;
    int m_PreviousViewport[4] = {0, 0, 0, 0};
};

} // namespace Wankel
