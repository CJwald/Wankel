#include "wkpch.h"
#include "Wankel/Particles/ParticleRenderer.h"

#include "Wankel/Particles/Particle.h"
#include "Wankel/Particles/ParticleEffect.h"
#include "Wankel/Renderer/Buffer.h"
#include "Wankel/Renderer/Camera.h"
#include "Wankel/Renderer/IndexBuffer.h"
#include "Wankel/Renderer/Shader.h"
#include "Wankel/Renderer/Texture.h"
#include "Wankel/Renderer/VertexArray.h"
#include "Wankel/Renderer/VertexBufferLayout.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace Wankel {

ParticleRenderer::ParticleRenderer(uint32_t maxParticles) : m_MaxParticles(maxParticles) {
    // UNIT QUAD (locations 0-1) - shared by every billboard, expanded to face the camera in
    // particle.vert. Interleaved as [corner.xy, uv.xy].
    const float quad[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, //
        0.5f,  -0.5f, 1.0f, 0.0f, //
        0.5f,  0.5f,  1.0f, 1.0f, //
        -0.5f, 0.5f,  0.0f, 1.0f, //
    };
    const uint32_t indices[] = {0, 1, 2, 2, 3, 0};

    m_VAO = CreateScope<VertexArray>();
    m_QuadVBO = CreateScope<VertexBuffer>(quad, sizeof(quad));
    VertexBufferLayout layout;
    layout.PushFloat(2, "aCorner");
    layout.PushFloat(2, "aUV");
    m_QuadVBO->SetLayout(layout);
    m_VAO->AddVertexBuffer(*m_QuadVBO); // also binds the VAO - the instance attribs below record into it

    // PER-INSTANCE BUFFER (locations 2-5) - one InstanceData per live particle, refilled each frame.
    // Set up by hand because VertexBufferLayout/VertexArray have no glVertexAttribDivisor path (same
    // reason Renderer::SubmitInstanced and ChunkGeometryPool wire their instance attribs directly).
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)maxParticles * sizeof(InstanceData)), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, Center));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, Size));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, Color));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, Rotation));
    glVertexAttribDivisor(5, 1);

    m_QuadIBO = CreateScope<IndexBuffer>(indices, 6);
    m_VAO->SetIndexBuffer(*m_QuadIBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_Shader = CreateScope<Shader>("WankelShaders/particle.vert", "WankelShaders/particle.frag");

    // BUILT-IN SOFT SPRITE - a radial alpha falloff, R8 (all the engine's Texture supports today).
    // Per-particle rgba comes from the instance color; this only shapes the alpha. A real RGBA/atlas
    // texture path is future work (ParticleEffect::TexturePath).
    constexpr uint32_t kSize = 64;
    std::vector<uint8_t> pixels(static_cast<size_t>(kSize) * kSize);
    for (uint32_t y = 0; y < kSize; y++) {
        for (uint32_t x = 0; x < kSize; x++) {
            float dx = ((float)x + 0.5f) / kSize * 2.0f - 1.0f;
            float dy = ((float)y + 0.5f) / kSize * 2.0f - 1.0f;
            float d = std::sqrt(dx * dx + dy * dy);
            float a = glm::clamp(1.0f - d, 0.0f, 1.0f);
            a = a * a * (3.0f - 2.0f * a); // smoothstep
            pixels[static_cast<size_t>(y) * kSize + x] = (uint8_t)(a * 255.0f);
        }
    }
    m_Sprite = CreateScope<Texture>(pixels.data(), kSize, kSize);
}

ParticleRenderer::~ParticleRenderer() {
    glDeleteBuffers(1, &m_InstanceVBO);
    // m_VAO / m_QuadVBO / m_QuadIBO / m_Shader / m_Sprite free their own GL objects (Scope / RAII).
}

void ParticleRenderer::Render(const Particle* particles, uint32_t count, const Camera& camera) {
    m_AlphaInstances.clear();
    m_AdditiveInstances.clear();

    for (uint32_t i = 0; i < count; i++) {
        const Particle& p = particles[i];
        float t = p.Lifetime > 0.0f ? glm::clamp(p.Age / p.Lifetime, 0.0f, 1.0f) : 1.0f;

        InstanceData d;
        d.Center = p.Position;
        d.Size = glm::mix(p.StartSize, p.EndSize, t);
        d.Color = glm::mix(p.StartColor, p.EndColor, t);
        d.Rotation = p.Rotation;

        if (p.Blend == (uint8_t)ParticleBlend::Additive)
            m_AdditiveInstances.push_back(d);
        else
            m_AlphaInstances.push_back(d);
    }

    if (m_AlphaInstances.empty() && m_AdditiveInstances.empty())
        return;

    // Coarse back-to-front sort so the standard alpha blend composites sanely; additive is
    // order-independent so it's left unsorted.
    const glm::vec3 camPos = camera.GetPosition();
    std::sort(m_AlphaInstances.begin(), m_AlphaInstances.end(), [&](const InstanceData& a, const InstanceData& b) {
        return glm::dot(a.Center - camPos, a.Center - camPos) > glm::dot(b.Center - camPos, b.Center - camPos);
    });

    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", camera.GetProjectionMatrix() * camera.GetViewMatrix());
    m_Shader->SetVec3("u_CameraRight", camera.GetRight());
    m_Shader->SetVec3("u_CameraUp", camera.GetUp());
    m_Shader->SetInt("u_Sprite", 0);
    m_Sprite->Bind(0);
    m_VAO->Bind();

    // Particles test against the depth buffer (world geometry occludes them) but don't write it -
    // they're only coarsely sorted and would otherwise carve holes in each other. Blend func is
    // restored to the engine-wide default (Renderer::Init) afterward.
    glDepthMask(GL_FALSE);
    DrawBin(m_AlphaInstances, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DrawBin(m_AdditiveInstances, GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ParticleRenderer::DrawBin(const std::vector<InstanceData>& bin, uint32_t glSrcFactor, uint32_t glDstFactor) {
    if (bin.empty())
        return;

    uint32_t n = (uint32_t)bin.size();
    if (n > m_MaxParticles) {
        WK_CORE_WARNING("ParticleRenderer::DrawBin - {0} instances submitted, truncating to capacity ({1})", n,
                        m_MaxParticles);
        n = m_MaxParticles;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)((size_t)n * sizeof(InstanceData)), bin.data());

    glBlendFunc(glSrcFactor, glDstFactor);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (GLsizei)n);
}

} // namespace Wankel
