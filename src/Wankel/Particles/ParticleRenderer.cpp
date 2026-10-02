#include "wkpch.h"
#include "Wankel/Particles/ParticleRenderer.h"

#include "Wankel/Assets/AssetManager.h"
#include "Wankel/Particles/Particle.h"
#include "Wankel/Particles/ParticleEffect.h"
#include "Wankel/Renderer/Buffer.h"
#include "Wankel/Renderer/Camera.h"
#include "Wankel/Renderer/Renderer.h"
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

namespace {

// Tiling R8 value noise, two octaves - the erosion/distortion source. Generated once, so effects don't depend on
// a noise asset being present.
std::vector<uint8_t> TilingNoise(uint32_t size, uint32_t cells) {
    auto hash = [](uint32_t x, uint32_t y) {
        uint32_t h = x * 374761393u + y * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
    };
    auto valueNoise = [&](float x, float y, uint32_t period) {
        uint32_t x0 = (uint32_t)x, y0 = (uint32_t)y;
        float fx = x - (float)x0, fy = y - (float)y0;
        fx = fx * fx * (3.0f - 2.0f * fx);
        fy = fy * fy * (3.0f - 2.0f * fy);
        float a = hash(x0 % period, y0 % period), b = hash((x0 + 1) % period, y0 % period);
        float c = hash(x0 % period, (y0 + 1) % period), d = hash((x0 + 1) % period, (y0 + 1) % period);
        return glm::mix(glm::mix(a, b, fx), glm::mix(c, d, fx), fy);
    };
    std::vector<uint8_t> pixels((size_t)size * size);
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            float u = (float)x / (float)size * (float)cells, v = (float)y / (float)size * (float)cells;
            float n = valueNoise(u, v, cells) * 0.65f + valueNoise(u * 2.0f, v * 2.0f, cells * 2) * 0.35f;
            pixels[(size_t)y * size + x] = (uint8_t)(glm::clamp(n, 0.0f, 1.0f) * 255.0f);
        }
    }
    return pixels;
}

// Atlas sub-rect (y from the image top) + flipbook frame -> GL UV rect (u0, v0, du, dv), v measured from the
// bottom since textures load flipped (Texture::LoadFromFile).
glm::vec4 FrameUVRect(const ParticleMaterial& material, const Particle& p, float t) {
    glm::vec4 atlas = material.AtlasRect;
    const ParticleFlipbook& fb = material.Flipbook;
    if (!fb.Enabled)
        return {atlas.x, 1.0f - atlas.y - atlas.w, atlas.z, atlas.w};

    uint32_t frames = fb.FrameCount;
    uint32_t frame;
    if (fb.FramesPerSecond > 0.0f) {
        frame = (uint32_t)(p.Age * fb.FramesPerSecond);
        frame = fb.Loop ? frame % frames : std::min(frame, frames - 1);
    } else {
        frame = std::min((uint32_t)(t * (float)frames), frames - 1);
    }
    if (fb.RandomStartFrame)
        frame = (frame + (uint32_t)(p.Seed * (float)frames)) % frames;

    float cellW = atlas.z / (float)fb.Columns;
    float cellH = atlas.w / (float)fb.Rows;
    uint32_t col = frame % fb.Columns;
    uint32_t row = frame / fb.Columns;
    float top = atlas.y + (float)row * cellH;
    return {atlas.x + (float)col * cellW, 1.0f - top - cellH, cellW, cellH};
}

} // namespace

ParticleRenderer::ParticleRenderer(uint32_t maxParticles) : m_MaxParticles(maxParticles) {
    // UNIT QUAD (locations 0-1) - shared by every particle, expanded in particle.vert. Interleaved as
    // [corner.xy, uv.xy]; corner.x is the long axis for the velocity-aligned modes.
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

    // PER-INSTANCE BUFFER (locations 2-7) - set up by hand because VertexBufferLayout/VertexArray have no
    // glVertexAttribDivisor path (same reason Renderer::SubmitInstanced and ChunkGeometryPool do).
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)maxParticles * sizeof(InstanceData)), nullptr, GL_DYNAMIC_DRAW);

    struct Attribute {
        GLuint Location;
        GLint Components;
        size_t Offset;
    };
    const Attribute attributes[] = {
        {2, 3, offsetof(InstanceData, Center)},
        {3, 1, offsetof(InstanceData, Size)},
        {4, 3, offsetof(InstanceData, Velocity)},
        {5, 1, offsetof(InstanceData, Rotation)},
        {6, 4, offsetof(InstanceData, Color)},
        {7, 4, offsetof(InstanceData, UVRect)},
        {8, 2, offsetof(InstanceData, ErosionThreshold)},
    };
    for (const Attribute& a : attributes) {
        glEnableVertexAttribArray(a.Location);
        glVertexAttribPointer(a.Location, a.Components, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)a.Offset);
        glVertexAttribDivisor(a.Location, 1);
    }

    m_QuadIBO = CreateScope<IndexBuffer>(indices, 6);
    m_VAO->SetIndexBuffer(*m_QuadIBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_Shader = CreateScope<Shader>("WankelShaders/particle.vert", "WankelShaders/particle.frag");

    // BUILT-IN SOFT SPRITE - white with a radial smoothstep alpha falloff, so a layer with no texture still
    // gets its rgba purely from the color gradient.
    constexpr uint32_t kSize = 64;
    std::vector<uint8_t> pixels(static_cast<size_t>(kSize) * kSize * 4);
    for (uint32_t y = 0; y < kSize; y++) {
        for (uint32_t x = 0; x < kSize; x++) {
            float dx = ((float)x + 0.5f) / kSize * 2.0f - 1.0f;
            float dy = ((float)y + 0.5f) / kSize * 2.0f - 1.0f;
            float a = glm::clamp(1.0f - std::sqrt(dx * dx + dy * dy), 0.0f, 1.0f);
            a = a * a * (3.0f - 2.0f * a); // smoothstep
            uint8_t* px = &pixels[(static_cast<size_t>(y) * kSize + x) * 4];
            px[0] = px[1] = px[2] = 255;
            px[3] = (uint8_t)(a * 255.0f);
        }
    }
    m_DefaultSprite = CreateScope<Texture>(pixels.data(), kSize, kSize, TextureFormat::RGBA8, true);

    constexpr uint32_t kNoiseSize = 128;
    std::vector<uint8_t> noise = TilingNoise(kNoiseSize, 8);
    m_Noise = CreateScope<Texture>(noise.data(), kNoiseSize, kNoiseSize, TextureFormat::R8, true);
    m_Noise->SetRepeat(true);
}

ParticleRenderer::~ParticleRenderer() {
    glDeleteBuffers(1, &m_InstanceVBO);
    // m_VAO / m_QuadVBO / m_QuadIBO / m_Shader / m_DefaultSprite free their own GL objects (Scope / RAII).
}

ParticleRenderer::FrameStats ParticleRenderer::Render(const Particle* particles, uint32_t count,
                                                      const std::vector<ParticleSystem::LayerSlot>& slots,
                                                      const Camera& camera, float time) {
    FrameStats stats;
    m_AlphaItems.clear();
    m_AdditiveItems.clear();

    const glm::vec3 camPos = camera.GetPosition();
    for (uint32_t i = 0; i < count; i++) {
        const Particle& p = particles[i];
        const ParticleLayer* layer = slots[p.LayerSlot].Layer();
        if (!layer)
            continue;
        float t = p.Lifetime > 0.0f ? glm::clamp(p.Age / p.Lifetime, 0.0f, 1.0f) : 1.0f;
        const ParticleAppearance& look = layer->Appearance;

        DrawItem item;
        item.Slot = p.LayerSlot;
        item.Data.Center = p.Position;
        item.Data.Size = p.Size * look.SizeOverLife.Evaluate(t);
        item.Data.Velocity = p.Velocity;
        item.Data.Rotation = p.Rotation;
        item.Data.Color = look.ColorOverLife.Evaluate(t);
        item.Data.Color.a *= look.AlphaOverLife.Evaluate(t);
        item.Data.UVRect = FrameUVRect(layer->Material, p, t);
        item.Data.ErosionThreshold = layer->Material.Erosion ? layer->Material.ErosionOverLife.Evaluate(t) : 0.0f;
        item.Data.Seed = p.Seed;

        if (layer->Material.Blend == ParticleBlend::Additive) {
            m_AdditiveItems.push_back(item);
        } else {
            glm::vec3 d = p.Position - camPos;
            item.SortKey = glm::dot(d, d);
            m_AlphaItems.push_back(item);
        }
    }

    stats.AlphaParticles = (uint32_t)m_AlphaItems.size();
    stats.AdditiveParticles = (uint32_t)m_AdditiveItems.size();
    if (m_AlphaItems.empty() && m_AdditiveItems.empty())
        return stats;

    // Alpha: back-to-front so the standard blend composites sanely, then split wherever the layer changes.
    // Additive: order-independent, so just grouped by layer to minimize draw calls.
    std::sort(m_AlphaItems.begin(), m_AlphaItems.end(),
              [](const DrawItem& a, const DrawItem& b) { return a.SortKey > b.SortKey; });
    std::sort(m_AdditiveItems.begin(), m_AdditiveItems.end(),
              [](const DrawItem& a, const DrawItem& b) { return a.Slot < b.Slot; });

    m_Instances.clear();
    m_Runs.clear();
    BuildRuns(m_AlphaItems, false);
    BuildRuns(m_AdditiveItems, true);

    uint32_t total = std::min((uint32_t)m_Instances.size(), m_MaxParticles);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)((size_t)total * sizeof(InstanceData)), m_Instances.data());

    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", camera.GetProjectionMatrix() * camera.GetViewMatrix());
    m_Shader->SetVec3("u_CameraRight", camera.GetRight());
    m_Shader->SetVec3("u_CameraUp", camera.GetUp());
    m_Shader->SetVec3("u_CameraPos", camPos);
    m_Shader->SetInt("u_Texture", 0);
    m_Shader->SetInt("u_Noise", 1);
    m_Shader->SetInt("u_SceneDepth", 2);
    m_Shader->SetFloat("u_Time", time);
    m_Shader->SetVec3("u_NearFar", glm::vec3(camera.GetNearClip(), camera.GetFarClip(), 0.0f));
    m_Noise->Bind(1);

    // Scene depth only gets copied on frames where a visible layer actually wants soft particles.
    bool anySoft = std::any_of(m_Runs.begin(), m_Runs.end(), [&](const Run& run) {
        const ParticleLayer* layer = slots[run.Slot].Layer();
        return layer && layer->Material.SoftParticles;
    });
    if (anySoft) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, Renderer::CaptureSceneDepth());
        glActiveTexture(GL_TEXTURE0);
    }
    m_VAO->Bind();

    // Particles test against the depth buffer (world geometry occludes them) but don't write it -
    // they're only coarsely sorted and would otherwise carve holes in each other. Blend func is
    // restored to the engine-wide default (Renderer::Init) afterward.
    // Flat quads - never cull them, whichever way a velocity-aligned one ends up wound.
    GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    for (const Run& run : m_Runs) {
        if (run.First >= total)
            break;
        DrawRun(run, slots);
        stats.DrawCalls++;
    }
    glDepthMask(GL_TRUE);
    if (cullWasEnabled)
        glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return stats;
}

void ParticleRenderer::BuildRuns(std::vector<DrawItem>& items, bool additive) {
    for (const DrawItem& item : items) {
        if (m_Runs.empty() || m_Runs.back().Slot != item.Slot || m_Runs.back().Additive != additive)
            m_Runs.push_back({item.Slot, (uint32_t)m_Instances.size(), 0, additive});
        m_Runs.back().Count++;
        m_Instances.push_back(item.Data);
    }
}

void ParticleRenderer::DrawRun(const Run& run, const std::vector<ParticleSystem::LayerSlot>& slots) {
    const ParticleLayer* layer = slots[run.Slot].Layer();
    if (!layer)
        return;
    const ParticleMaterial& material = layer->Material;
    const ParticleAppearance& look = layer->Appearance;

    Ref<Texture> texture = material.TexturePath.empty() ? nullptr : AssetManager::GetTexture(material.TexturePath);
    if (texture)
        texture->Bind(0);
    else
        m_DefaultSprite->Bind(0);

    m_Shader->SetInt("u_Orientation", (int)look.Orientation);
    m_Shader->SetFloat("u_Aspect", look.Aspect);
    m_Shader->SetFloat("u_StretchFactor", look.StretchFactor);
    m_Shader->SetFloat("u_Emissive", material.EmissiveStrength);
    m_Shader->SetInt("u_Soft", material.SoftParticles ? 1 : 0);
    m_Shader->SetFloat("u_SoftDistance", material.SoftDistance);
    m_Shader->SetInt("u_Erosion", material.Erosion ? 1 : 0);
    m_Shader->SetFloat("u_ErosionSoftness", material.ErosionSoftness);
    m_Shader->SetInt("u_Distortion", material.Distortion ? 1 : 0);
    m_Shader->SetVec3("u_DistortionParams",
                      glm::vec3(material.DistortionStrength, material.DistortionScale, material.DistortionScroll));

    if (run.Additive)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t n = std::min(run.Count, m_MaxParticles - run.First);
    glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (GLsizei)n, run.First);
}

} // namespace Wankel
