#include "wkpch.h"
#include "ShadowMap.h"

#include <glad/gl.h>

#include <algorithm>

namespace Wankel {

ShadowMap::ShadowMap(uint32_t resolution) : m_Resolution(std::clamp(resolution, 1u, 8192u)) {
    glGenFramebuffers(1, &m_Framebuffer);
    Allocate();
}

ShadowMap::~ShadowMap() {
    glDeleteTextures(1, &m_DepthTexture);
    glDeleteFramebuffers(1, &m_Framebuffer);
}

void ShadowMap::Resize(uint32_t resolution) {
    resolution = std::clamp(resolution, 1u, 8192u);
    if (resolution == m_Resolution)
        return;
    m_Resolution = resolution;
    Allocate();
}

void ShadowMap::Allocate() {
    if (m_DepthTexture)
        glDeleteTextures(1, &m_DepthTexture);

    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, (GLsizei)m_Resolution, (GLsizei)m_Resolution, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        WK_CORE_ERROR("ShadowMap - framebuffer incomplete at {0}x{0}", m_Resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Wankel
