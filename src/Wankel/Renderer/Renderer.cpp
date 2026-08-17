#include "wkpch.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Font.h"
#include "Texture.h"
#include "OcclusionQuery.h"

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
    std::array<PointLightGPU, kMaxPointLights> PointLights;
    int PointLightCount = 0;

    std::vector<DebugVertex> DebugVertices;
    // Same vertex format/shader/GL objects as DebugVertices, drawn every frame regardless of
    // DebugEnabled - see SubmitGameplayLines.
    std::vector<DebugVertex> GameplayLineVertices;
    uint32_t DebugVAO = 0;
    uint32_t DebugVBO = 0;
    Shader* DebugShader = nullptr;

    uint32_t TextVAO = 0;
    uint32_t TextVBO = 0;
    Shader* TextShader = nullptr;

    // Screen-space filled rectangle pass (SubmitScreenQuad) - own shader rather than reusing
    // DebugShader, since its color/alpha are set via uniforms (not per-vertex like DebugVertex), and
    // a shared uniform mutated per-call would otherwise leak into unrelated SubmitDebugLines/
    // SubmitScreenLines draws using the same shader object unless carefully restored every time.
    uint32_t ScreenQuadVAO = 0;
    uint32_t ScreenQuadVBO = 0;
    Shader* ScreenQuadShader = nullptr;

    uint32_t InstanceVBO = 0; // shared across every SubmitInstanced call - see Init()

    // Submit() dedup state - reset each BeginScene so a mid-scene SetFog/SetLight change still
    // lands on the next Submit even if the shader/material pointer/value hasn't changed.
    Shader* LastSubmitShader = nullptr;
    Material LastMaterial {};
    bool LastMaterialValid = false;
};


static RendererData s_Data;
static constexpr size_t kMaxDebugVertices = 65536;
static constexpr size_t kMaxTextVertices = 256 * 6; // 256 glyphs/quads per SubmitText call
static constexpr size_t kMaxInstancesPerDraw =
    4096; // generous headroom over typical simultaneously-visible tile counts


void Renderer::Init() {
    glEnable(GL_DEPTH_TEST);
    // LEQUAL (not the default LESS) so a conditionally-rendered second pass over the same geometry
    // at the same depth (see BeginConditionalRender) doesn't fail its own depth test - see
    // OcclusionQuery.h/the occlusion-culling render-loop comments for why this pairing is needed.
    glDepthFunc(GL_LEQUAL);
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

    // SCREEN QUAD PASS GPU OBJECTS (SubmitScreenQuad) - position-only, 2 triangles (6 verts) per call,
    // color/alpha come from uniforms instead of per-vertex attributes.
    glGenVertexArrays(1, &s_Data.ScreenQuadVAO);
    glGenBuffers(1, &s_Data.ScreenQuadVBO);
    glBindVertexArray(s_Data.ScreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.ScreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 6, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    s_Data.ScreenQuadShader = new Shader("WankelShaders/screenquad.vert", "WankelShaders/screenquad.frag");

    // SHARED INSTANCE-OFFSET BUFFER (SubmitInstanced) - re-bound as a per-instance attribute onto
    // whichever mesh VAO is current at draw time, refreshed via glBufferSubData each call, same
    // "allocate once in Init, glBufferSubData per use" pattern as DebugVBO/TextVBO above.
    glGenBuffers(1, &s_Data.InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * kMaxInstancesPerDraw, nullptr, GL_DYNAMIC_DRAW);
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

    delete s_Data.ScreenQuadShader;
    s_Data.ScreenQuadShader = nullptr;

    glDeleteBuffers(1, &s_Data.ScreenQuadVBO);
    glDeleteVertexArrays(1, &s_Data.ScreenQuadVAO);

    glDeleteBuffers(1, &s_Data.InstanceVBO);
}


void Renderer::BeginScene(const Camera& camera) {
    s_Data.View = camera.GetViewMatrix();
    s_Data.Projection = camera.GetProjectionMatrix();
    s_Data.CameraPos = camera.GetPosition();
    s_Data.DebugVertices.clear();
    s_Data.GameplayLineVertices.clear();
    s_Data.LastSubmitShader = nullptr;
    s_Data.LastMaterialValid = false;
}


namespace {

// Shared by both line queues below - same shader/VAO/VBO either way, just a different vertex source.
void FlushLineQueue(const std::vector<DebugVertex>& vertices) {
    if (vertices.empty())
        return;

    size_t vertexCount = vertices.size();
    if (vertexCount > kMaxDebugVertices) {
        WK_CORE_WARNING("Renderer::EndScene - {0} line vertices submitted, truncating to capacity ({1})", vertexCount,
                        kMaxDebugVertices);
        vertexCount = kMaxDebugVertices;
    }

    s_Data.DebugShader->Bind();
    s_Data.DebugShader->SetMat4("view", s_Data.View);
    s_Data.DebugShader->SetMat4("projection", s_Data.Projection);

    glBindVertexArray(s_Data.DebugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(DebugVertex), vertices.data());
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
    glEnable(GL_CULL_FACE);
}

} // namespace

void Renderer::EndScene() {
    // DEBUG PASS - gated, only visible with the "Debug Draw" toggle on.
    if (DebugEnabled)
        FlushLineQueue(s_Data.DebugVertices);

    // GAMEPLAY PASS - always drawn, see SubmitGameplayLines.
    FlushLineQueue(s_Data.GameplayLineVertices);
}


namespace {

// Shared by Submit/SubmitInstanced: shader bind, frame-constant uniform dedup, per-draw model/normal
// matrix, and material dedup. Everything after this differs (mesh bind + the actual draw call).
void UploadPerDrawState(Shader* shader, const glm::mat4& transform, const Mesh& mesh, const Material& material,
                        bool useVertexColor) {
    shader->Bind(); // no-op if already the current program

    // Frame-constant uniforms (view/projection/camera/light/fog/time) only need re-uploading to a
    // given shader once per frame, not once per draw - re-set them only the first time this
    // particular shader is used since BeginScene.
    if (shader != s_Data.LastSubmitShader) {
        shader->SetMat4("view", s_Data.View);
        shader->SetMat4("projection", s_Data.Projection);
        shader->SetVec3("u_CameraPos", s_Data.CameraPos);

        shader->SetVec3("u_LightDir", s_Data.Light.Direction);
        shader->SetVec3("u_LightColor", s_Data.Light.Color);
        shader->SetFloat("u_AmbientStrength", s_Data.Light.Ambient);
        shader->SetFloat("u_SpecularStrength", s_Data.Light.Specular);

        shader->SetInt("u_PointLightCount", s_Data.PointLightCount);
        for (int i = 0; i < s_Data.PointLightCount; ++i) {
            std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
            shader->SetVec3(prefix + "Position", s_Data.PointLights[i].Position);
            shader->SetVec3(prefix + "Color", s_Data.PointLights[i].Color);
            shader->SetFloat(prefix + "Intensity", s_Data.PointLights[i].Intensity);
            shader->SetFloat(prefix + "Radius", s_Data.PointLights[i].Radius);
        }

        shader->SetVec3("u_FogColor", s_Data.Fog.Color);
        shader->SetFloat("u_FogDensity", s_Data.Fog.Density);
        shader->SetFloat("u_Time", Time::GetTime());

        shader->SetFloat("u_FogNoiseScale", s_Data.Fog.NoiseScale);
        shader->SetFloat("u_FogNoiseStrength", s_Data.Fog.NoiseStrength);
        shader->SetInt("u_FogNoiseOctaves", s_Data.Fog.NoiseOctaves);
        shader->SetInt("u_FogNoiseEnabled", s_Data.Fog.NoiseEnabled ? 1 : 0);
        shader->SetVec3("u_FogWindDir", s_Data.Fog.WindDir);
        shader->SetFloat("u_FogWindSpeed", s_Data.Fog.WindSpeed);

        s_Data.LastSubmitShader = shader;
        s_Data.LastMaterialValid = false; // this program hasn't seen a material upload yet this frame
    }

    // Not frame-constant, so not part of the dedup block above - Submit and SubmitInstanced can
    // alternate within the same frame using the same shader (player/enemy vs. terrain chunks in
    // MechtrixLayer's render loop), so this must be set on every draw, not just the shader's first
    // use this frame. Gates whether the fragment shader multiplies Vertex::Color into the surface
    // color - see Submit/SubmitInstanced's own call sites for which is which.
    shader->SetInt("u_UseVertexColor", useVertexColor ? 1 : 0);

    // Position-quantized meshes (Mesh's quantizing ctor) store [0,1]-normalized positions - folding
    // the dequantization into `model` here means the vertex shader needs no changes at all;
    // identity ({0,0,0}/{1,1,1}) for a non-quantized mesh, so this is always safe to apply. Must NOT
    // feed into u_NormalMatrix below - that's a packing-artifact correction on position only, not a
    // real transform of the object, and normals are already correct relative to the original
    // (pre-quantization) local space.
    glm::mat4 model = transform * glm::translate(glm::mat4(1.0f), mesh.GetQuantizeMin()) *
                      glm::scale(glm::mat4(1.0f), mesh.GetQuantizeExtent());
    shader->SetMat4("model", model);
    shader->SetMat3("u_NormalMatrix", glm::inverseTranspose(glm::mat3(transform)));

    // Material uniforms only need re-uploading when they actually differ from the last draw's -
    // e.g. every voxel chunk shares one identical Material, so this collapses to one upload total.
    if (!s_Data.LastMaterialValid || !(material == s_Data.LastMaterial)) {
        shader->SetVec3("u_Albedo", material.Albedo);
        shader->SetFloat("u_Roughness", material.Roughness);
        shader->SetFloat("u_Metallic", material.Metallic);
        shader->SetVec3("u_Emissive", material.Emissive);
        s_Data.LastMaterial = material;
        s_Data.LastMaterialValid = true;
    }
}

} // namespace

void Renderer::Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader, const Material& material) {
    UploadPerDrawState(shader, transform, mesh, material, /*useVertexColor=*/false);

    mesh.Bind(); // no-op if already the current VAO

    glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::SubmitInstanced(const glm::mat4& transform, const Mesh& mesh, Shader* shader, const Material& material,
                               const std::vector<glm::vec3>& instanceOffsets) {
    if (instanceOffsets.empty())
        return;

    // Exclusively used for voxel terrain today - real per-voxel color (see VoxelMesher) lives in
    // Vertex::Color, so this is the one draw path that wants it multiplied in.
    UploadPerDrawState(shader, transform, mesh, material, /*useVertexColor=*/true);

    mesh.Bind(); // binds this chunk's VAO - the attribute setup below applies to it, not whatever was bound before

    size_t count = instanceOffsets.size();
    if (count > kMaxInstancesPerDraw) {
        WK_CORE_WARNING("Renderer::SubmitInstanced - {0} instances submitted, truncating to capacity ({1})", count,
                        kMaxInstancesPerDraw);
        count = kMaxInstancesPerDraw;
    }

    glBindBuffer(GL_ARRAY_BUFFER, s_Data.InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(glm::vec3), instanceOffsets.data());
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glVertexAttribDivisor(3, 1);

    glDrawElementsInstanced(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr, (GLsizei)count);
}


void Renderer::SetColorWrite(bool enabled) {
    glColorMask(enabled, enabled, enabled, enabled);
}

void Renderer::BeginOcclusionQuery(const OcclusionQuery& query) {
    glBeginQuery(GL_ANY_SAMPLES_PASSED, query.GetID());
}

void Renderer::EndOcclusionQuery() {
    glEndQuery(GL_ANY_SAMPLES_PASSED);
}

void Renderer::BeginConditionalRender(const OcclusionQuery& query) {
    // NO_WAIT: if the result isn't ready yet (e.g. a chunk queried for the first time this frame),
    // render anyway rather than stalling for it - safe/conservative, never wrongly hides geometry.
    glBeginConditionalRender(query.GetID(), GL_QUERY_NO_WAIT);
}

void Renderer::EndConditionalRender() {
    glEndConditionalRender();
}


void Renderer::SubmitDebugLines(const std::vector<DebugLine>& lines) {
    if (!DebugEnabled)
        return;

    for (const auto& line : lines) {
        s_Data.DebugVertices.push_back({line.P0, line.Color});
        s_Data.DebugVertices.push_back({line.P1, line.Color});
    }
}

void Renderer::SubmitGameplayLines(const std::vector<DebugLine>& lines) {
    for (const auto& line : lines) {
        s_Data.GameplayLineVertices.push_back({line.P0, line.Color});
        s_Data.GameplayLineVertices.push_back({line.P1, line.Color});
    }
}

void Renderer::SubmitScreenLines(const std::vector<DebugLine>& lines, uint32_t screenWidth, uint32_t screenHeight) {
    if (lines.empty() || screenWidth == 0 || screenHeight == 0)
        return;

    std::vector<DebugVertex> vertices;
    vertices.reserve(lines.size() * 2);
    for (const auto& line : lines) {
        vertices.push_back({line.P0, line.Color});
        vertices.push_back({line.P1, line.Color});
    }

    size_t vertexCount = vertices.size();
    if (vertexCount > kMaxDebugVertices) {
        WK_CORE_WARNING("Renderer::SubmitScreenLines - {0} vertices submitted, truncating to capacity ({1})",
                        vertexCount, kMaxDebugVertices);
        vertexCount = kMaxDebugVertices;
    }

    // Same DebugVAO/DebugVBO/DebugShader as the 3D line queues, just fed an orthographic pixel-space
    // projection and an identity view instead - reuses the shader's `projection * view * position`
    // transform for 2D screen space rather than adding a second shader/GL object set.
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);

    s_Data.DebugShader->Bind();
    s_Data.DebugShader->SetMat4("view", glm::mat4(1.0f));
    s_Data.DebugShader->SetMat4("projection", projection);

    glBindVertexArray(s_Data.DebugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.DebugVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(DebugVertex), vertices.data());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
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


void Renderer::SubmitScreenQuad(const glm::vec2& min, const glm::vec2& max, const glm::vec3& color, float alpha,
                                uint32_t screenWidth, uint32_t screenHeight) {
    // Same 0x0-framebuffer guard as SubmitText - see its own comment.
    if (screenWidth == 0 || screenHeight == 0)
        return;

    glm::vec3 vertices[6] = {
        {min.x, min.y, 0.0f}, {max.x, min.y, 0.0f}, {max.x, max.y, 0.0f},
        {min.x, min.y, 0.0f}, {max.x, max.y, 0.0f}, {min.x, max.y, 0.0f},
    };

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);

    s_Data.ScreenQuadShader->Bind();
    s_Data.ScreenQuadShader->SetMat4("view", glm::mat4(1.0f));
    s_Data.ScreenQuadShader->SetMat4("projection", projection);
    s_Data.ScreenQuadShader->SetVec3("u_Color", color);
    s_Data.ScreenQuadShader->SetFloat("u_Alpha", alpha);

    glBindVertexArray(s_Data.ScreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.ScreenQuadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    // Screen-space overlay, same reasoning as SubmitText's own comment: the ortho Y-flip makes this
    // quad's winding come out clockwise, which would otherwise be back-face culled.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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

void Renderer::SetPointLights(const std::vector<PointLightGPU>& lights) {
    size_t count = lights.size();
    if (count > kMaxPointLights) {
        WK_CORE_WARNING("Renderer::SetPointLights - {0} point lights submitted, truncating to capacity ({1})", count,
                        kMaxPointLights);
        count = kMaxPointLights;
    }

    for (size_t i = 0; i < count; ++i)
        s_Data.PointLights[i] = lights[i];
    s_Data.PointLightCount = (int)count;
}

} // namespace Wankel
