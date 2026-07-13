#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Font.h"
#include "Texture.h"

#include "Wankel/Core/Time.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Wankel {


struct DebugVertex {
    glm::vec3 Position;
    glm::vec3 Color;
};


struct TextVertex {
    glm::vec2 Position;
    glm::vec2 UV;
};


bool Renderer::DebugEnabled = false;


struct RendererData {
    glm::mat4 View;
    glm::mat4 Projection;
    glm::vec3 CameraPos;

    FogSettings Fog;
    LightSettings Light;

    std::vector<DebugVertex> DebugVertices;
    uint32_t DebugVAO = 0;
    uint32_t DebugVBO = 0;
    Shader* DebugShader = nullptr;

    uint32_t TextVAO = 0;
    uint32_t TextVBO = 0;
    Shader* TextShader = nullptr;
};


static RendererData s_Data;
static constexpr size_t kMaxDebugVertices = 65536;
static constexpr size_t kMaxTextVertices = 256 * 6; // 256 glyphs/quads per SubmitText call


void Renderer::Init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);

    // DEBUG PASS GPU OBJECTS
    glGenVertexArrays(1, &s_Data.DebugVAO);
    glGenBuffers(1, &s_Data.DebugVBO);
    glBindVertexArray(s_Data.DebugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * kMaxDebugVertices, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, Color));
    s_Data.DebugShader = new Shader("WankelShaders/debug.vert", "WankelShaders/debug.frag");

    // TEXT PASS GPU OBJECTS
    glGenVertexArrays(1, &s_Data.TextVAO);
    glGenBuffers(1, &s_Data.TextVBO);
    glBindVertexArray(s_Data.TextVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.TextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TextVertex) * kMaxTextVertices, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, UV));
    s_Data.TextShader = new Shader("WankelShaders/text.vert", "WankelShaders/text.frag");
}


void Renderer::Shutdown() {
    delete s_Data.DebugShader;
    s_Data.DebugShader = nullptr;

    glDeleteBuffers(1, &s_Data.DebugVBO);
    glDeleteVertexArrays(1, &s_Data.DebugVAO);

    delete s_Data.TextShader;
    s_Data.TextShader = nullptr;

    glDeleteBuffers(1, &s_Data.TextVBO);
    glDeleteVertexArrays(1, &s_Data.TextVAO);
}


void Renderer::BeginScene(const Camera& camera) {
    s_Data.View = camera.GetViewMatrix();
    s_Data.Projection = camera.GetProjectionMatrix();
    s_Data.CameraPos = camera.GetPosition();
    s_Data.DebugVertices.clear();
}


void Renderer::EndScene() {
    // DEBUG PASS
    if (DebugEnabled && !s_Data.DebugVertices.empty()) {
        size_t vertexCount = s_Data.DebugVertices.size();

        if (vertexCount > kMaxDebugVertices) {
            WK_CORE_WARNING("Renderer::EndScene - {0} debug vertices submitted, truncating to capacity ({1})",
                            vertexCount, kMaxDebugVertices);
            vertexCount = kMaxDebugVertices;
        }

        s_Data.DebugShader->Bind();
        s_Data.DebugShader->SetMat4("view", s_Data.View);
        s_Data.DebugShader->SetMat4("projection", s_Data.Projection);

        glBindVertexArray(s_Data.DebugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(DebugVertex), s_Data.DebugVertices.data());
        glDisable(GL_CULL_FACE);
        glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
        glEnable(GL_CULL_FACE);
    }
}


void Renderer::Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader) {
    shader->Bind();
    shader->SetMat4("view", s_Data.View);
    shader->SetMat4("projection", s_Data.Projection);
    shader->SetMat4("model", transform);
    shader->SetMat3("u_NormalMatrix", glm::inverseTranspose(glm::mat3(transform)));
    shader->SetVec3("u_CameraPos", s_Data.CameraPos);

    shader->SetVec3("u_LightDir", s_Data.Light.Direction);
    shader->SetVec3("u_LightColor", s_Data.Light.Color);
    shader->SetFloat("u_AmbientStrength", s_Data.Light.Ambient);
    shader->SetFloat("u_SpecularStrength", s_Data.Light.Specular);
    shader->SetFloat("u_Shininess", s_Data.Light.Shininess);

    shader->SetVec3("u_FogColor", s_Data.Fog.Color);
    shader->SetFloat("u_FogDensity", s_Data.Fog.Density);
    shader->SetFloat("u_Time", Time::GetTime());

    shader->SetFloat("u_FogNoiseScale", s_Data.Fog.NoiseScale);
    shader->SetFloat("u_FogNoiseStrength", s_Data.Fog.NoiseStrength);
    shader->SetInt("u_FogNoiseOctaves", s_Data.Fog.NoiseOctaves);
    shader->SetInt("u_FogNoiseEnabled", s_Data.Fog.NoiseEnabled ? 1 : 0);
    shader->SetVec3("u_FogWindDir", s_Data.Fog.WindDir);
    shader->SetFloat("u_FogWindSpeed", s_Data.Fog.WindSpeed);

    mesh.Bind();

    glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}


void Renderer::SubmitDebugLines(const std::vector<DebugLine>& lines) {
    if (!DebugEnabled)
        return;

    for (const auto& line : lines) {
        s_Data.DebugVertices.push_back({line.P0, line.Color});
        s_Data.DebugVertices.push_back({line.P1, line.Color});
    }
}


void Renderer::SubmitText(const std::string& text, const Ref<Font>& font, const glm::vec2& screenPos,
                           uint32_t screenWidth, uint32_t screenHeight, const glm::vec3& color) {
    if (!font || text.empty())
        return;

    // A 0-sized viewport (e.g. a minimized/iconified window - GLFW commonly
    // reports a 0x0 framebuffer in that state) would make glm::ortho divide
    // by (right-left)/(top-bottom) == 0, injecting Inf/NaN into the
    // projection matrix uploaded below.
    if (screenWidth == 0 || screenHeight == 0)
        return;

    std::vector<GlyphQuad> quads;
    font->BuildQuads(text, screenPos, quads);

    if (quads.empty())
        return;

    std::vector<TextVertex> vertices;
    vertices.reserve(quads.size() * 6);

    for (const auto& q : quads) {
        TextVertex v0 {{q.Min.x, q.Min.y}, {q.UVMin.x, q.UVMin.y}};
        TextVertex v1 {{q.Max.x, q.Min.y}, {q.UVMax.x, q.UVMin.y}};
        TextVertex v2 {{q.Max.x, q.Max.y}, {q.UVMax.x, q.UVMax.y}};
        TextVertex v3 {{q.Min.x, q.Max.y}, {q.UVMin.x, q.UVMax.y}};

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);

        vertices.push_back(v0);
        vertices.push_back(v2);
        vertices.push_back(v3);
    }

    size_t vertexCount = vertices.size();
    if (vertexCount > kMaxTextVertices) {
        WK_CORE_WARNING("Renderer::SubmitText - {0} vertices submitted, truncating to capacity ({1})", vertexCount,
                         kMaxTextVertices);
        vertexCount = kMaxTextVertices;
    }

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);

    s_Data.TextShader->Bind();
    s_Data.TextShader->SetMat4("u_Projection", projection);
    s_Data.TextShader->SetVec3("u_Color", color);
    font->GetAtlasTexture()->Bind(0);
    s_Data.TextShader->SetInt("u_FontAtlas", 0);

    glBindVertexArray(s_Data.TextVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.TextVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(TextVertex), vertices.data());

    // Screen-space overlay - not part of the depth-tested 3D scene, and the
    // ortho projection's Y-flip (screen Y-down -> NDC Y-up) makes the quad
    // winding come out clockwise in the final rasterized image, so it's
    // back-face culled under the engine's default GL_CULL_FACE/GL_BACK -
    // disable culling for this pass, there's no "back side" of 2D text
    // that should ever be culled anyway.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}


void Renderer::Draw(const Mesh& mesh) {
    mesh.Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}


void Renderer::Clear() {
    const auto& fog = s_Data.Fog;

    glClearColor(fog.Color.r, fog.Color.g, fog.Color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
    glViewport(0, 0, width, height);
}

void Renderer::SetFog(const FogSettings& fog) {
    s_Data.Fog = fog;
}

void Renderer::SetLight(const LightSettings& light) {
    s_Data.Light = light;
}

} // namespace Wankel
