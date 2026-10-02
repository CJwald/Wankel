#include "wkpch.h"
#include "RenderTarget.h"

#include <glad/gl.h>

#include <algorithm>

namespace Wankel {

RenderTarget::RenderTarget(uint32_t width, uint32_t height)
    : m_Width(std::max(width, 1u)), m_Height(std::max(height, 1u)) {
    glGenFramebuffers(1, &m_Framebuffer);
    Allocate();
}

RenderTarget::~RenderTarget() {
    glDeleteTextures(1, &m_Color);
    glDeleteTextures(1, &m_Depth);
    glDeleteFramebuffers(1, &m_Framebuffer);
}

void RenderTarget::Resize(uint32_t width, uint32_t height) {
    width = std::max(width, 1u);
    height = std::max(height, 1u);
    if (width == m_Width && height == m_Height)
        return;
    m_Width = width;
    m_Height = height;
    Allocate();
}

void RenderTarget::Allocate() {
    if (m_Color)
        glDeleteTextures(1, &m_Color);
    if (m_Depth)
        glDeleteTextures(1, &m_Depth);

    glGenTextures(1, &m_Color);
    glBindTexture(GL_TEXTURE_2D, m_Color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)m_Width, (GLsizei)m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_Depth);
    glBindTexture(GL_TEXTURE_2D, m_Depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)m_Width, (GLsizei)m_Height, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_Depth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        WK_CORE_ERROR("RenderTarget - framebuffer incomplete at {0}x{1}", m_Width, m_Height);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous);
}

void RenderTarget::Begin() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PreviousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, m_PreviousViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, (GLsizei)m_Width, (GLsizei)m_Height);
}

void RenderTarget::End() {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)m_PreviousFramebuffer);
    glViewport(m_PreviousViewport[0], m_PreviousViewport[1], m_PreviousViewport[2], m_PreviousViewport[3]);
}

} // namespace Wankel
