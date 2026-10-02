#include "wkpch.h"
#include "Wankel/Particles/Editor/ParticleEditor.h"

#include "Wankel/Assets/AssetManager.h"
#include "Wankel/Particles/ParticleEffect.h"
#include "Wankel/Particles/ParticleLibrary.h"
#include "Wankel/Renderer/DebugDraw.h"
#include "Wankel/Renderer/RenderTarget.h"
#include "Wankel/Renderer/Renderer.h"
#include "Wankel/Renderer/Texture.h"

#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace Wankel {

namespace {

constexpr float kStep = 1.0f / 60.0f; // scrub/fast-forward step

ImTextureID ToImTexture(uint32_t glTexture) {
    return (ImTextureID)(uintptr_t)glTexture;
}

// GL textures (loaded flipped, and render targets) have v=0 at the bottom; ImGui draws v=0 at the top.
const ImVec2 kUvTop(0.0f, 1.0f);
const ImVec2 kUvBottom(1.0f, 0.0f);

template <typename E, size_t N>
bool EnumCombo(const char* label, E& value, const char* const (&names)[N]) {
    int index = (int)value;
    if (!ImGui::Combo(label, &index, names, (int)N))
        return false;
    value = (E)index;
    return true;
}

bool InputString(const char* label, std::string& value) {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
    if (!ImGui::InputText(label, buffer, sizeof(buffer)))
        return false;
    value = buffer;
    return true;
}

// Draggable keyframe canvas plus a precise key list. Click-drag a key, double-click to add, right-click to remove.
bool CurveEditor(const char* label, ParticleCurve& curve, float minValue, float maxValue) {
    bool changed = false;
    ImGui::PushID(label);
    if (curve.Keys.empty())
        curve.Keys.push_back({0.0f, 1.0f});
    for (const auto& key : curve.Keys)
        maxValue = std::max(maxValue, key.Value);

    ImGui::TextUnformatted(label);
    float width = ImGui::GetContentRegionAvail().x;
    ImVec2 size(width, 70.0f);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##canvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(28, 28, 32, 255));

    auto toScreen = [&](float t, float v) {
        float y = (v - minValue) / std::max(maxValue - minValue, 1e-4f);
        return ImVec2(origin.x + t * size.x, origin.y + (1.0f - y) * size.y);
    };
    if (minValue < 0.0f && maxValue > 0.0f) {
        ImVec2 a = toScreen(0.0f, 0.0f), b = toScreen(1.0f, 0.0f);
        draw->AddLine(a, b, IM_COL32(70, 70, 76, 255));
    }
    constexpr int kSamples = 64;
    ImVec2 previous = toScreen(0.0f, curve.Evaluate(0.0f));
    for (int i = 1; i <= kSamples; i++) {
        float t = (float)i / kSamples;
        ImVec2 point = toScreen(t, curve.Evaluate(t));
        draw->AddLine(previous, point, IM_COL32(150, 205, 64, 255), 2.0f);
        previous = point;
    }

    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID dragId = ImGui::GetID("##drag");
    int dragging = storage->GetInt(dragId, -1);
    ImVec2 mouse = ImGui::GetIO().MousePos;
    int hovered = -1;
    for (int i = 0; i < (int)curve.Keys.size(); i++) {
        ImVec2 p = toScreen(curve.Keys[i].Time, curve.Keys[i].Value);
        bool near = std::abs(mouse.x - p.x) < 6.0f && std::abs(mouse.y - p.y) < 6.0f;
        if (near && ImGui::IsItemHovered())
            hovered = i;
        draw->AddCircleFilled(
            p, 4.0f, i == dragging || i == hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255));
    }

    if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        storage->SetInt(dragId, dragging = hovered);
    if (dragging >= 0 && dragging < (int)curve.Keys.size() && ImGui::IsItemActive() &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        float t = std::clamp((mouse.x - origin.x) / size.x, 0.0f, 1.0f);
        float v = minValue + (1.0f - (mouse.y - origin.y) / size.y) * (maxValue - minValue);
        float lo = dragging > 0 ? curve.Keys[dragging - 1].Time : 0.0f;
        float hi = dragging + 1 < (int)curve.Keys.size() ? curve.Keys[dragging + 1].Time : 1.0f;
        curve.Keys[dragging].Time = std::clamp(t, lo, hi);
        curve.Keys[dragging].Value = std::clamp(v, minValue, maxValue);
        changed = true;
    }
    if (!ImGui::IsItemActive())
        storage->SetInt(dragId, -1);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered < 0) {
        float t = std::clamp((mouse.x - origin.x) / size.x, 0.0f, 1.0f);
        curve.Keys.push_back({t, curve.Evaluate(t)});
        curve.Sort();
        changed = true;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && hovered >= 0 && curve.Keys.size() > 1) {
        curve.Keys.erase(curve.Keys.begin() + hovered);
        changed = true;
    }

    if (ImGui::TreeNode("Keys")) {
        for (int i = 0; i < (int)curve.Keys.size(); i++) {
            ImGui::PushID(i);
            float lo = i > 0 ? curve.Keys[i - 1].Time : 0.0f;
            float hi = i + 1 < (int)curve.Keys.size() ? curve.Keys[i + 1].Time : 1.0f;
            ImGui::SetNextItemWidth(70.0f);
            changed |=
                ImGui::DragFloat("##t", &curve.Keys[i].Time, 0.005f, lo, hi, "t %.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            changed |= ImGui::DragFloat("##v", &curve.Keys[i].Value, 0.01f);
            ImGui::SameLine();
            if (curve.Keys.size() > 1 && ImGui::SmallButton("x")) {
                curve.Keys.erase(curve.Keys.begin() + i);
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::SmallButton("+ Key")) {
            curve.Keys.push_back({1.0f, curve.Keys.back().Value});
            if (curve.Keys.size() >= 2)
                curve.Keys[curve.Keys.size() - 2].Time = std::min(curve.Keys[curve.Keys.size() - 2].Time, 0.99f);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Const 1")) {
            curve = ParticleCurve::Constant(1.0f);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("0 > 1")) {
            curve = ParticleCurve::Linear(0.0f, 1.0f);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("1 > 0")) {
            curve = ParticleCurve::Linear(1.0f, 0.0f);
            changed = true;
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
    return changed;
}

ImU32 ToU32(const glm::vec4& c) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, c.a));
}

void DrawChecker(ImDrawList* draw, ImVec2 min, ImVec2 max, float cell) {
    draw->AddRectFilled(min, max, IM_COL32(150, 150, 150, 255));
    for (float y = min.y; y < max.y; y += cell) {
        for (float x = min.x + (int((y - min.y) / cell) % 2) * cell; x < max.x; x += cell * 2.0f)
            draw->AddRectFilled(ImVec2(x, y), ImVec2(std::min(x + cell, max.x), std::min(y + cell, max.y)),
                                IM_COL32(100, 100, 100, 255));
    }
}

// Color bar over a checkerboard with draggable key markers underneath, plus a key list.
bool GradientEditor(const char* label, ParticleGradient& gradient) {
    bool changed = false;
    ImGui::PushID(label);
    if (gradient.Keys.empty())
        gradient.Keys.push_back({0.0f, glm::vec4(1.0f)});

    ImGui::TextUnformatted(label);
    float width = ImGui::GetContentRegionAvail().x;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 barMax(origin.x + width, origin.y + 22.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawChecker(draw, origin, barMax, 6.0f);
    constexpr int kSegments = 48;
    for (int i = 0; i < kSegments; i++) {
        float t0 = (float)i / kSegments, t1 = (float)(i + 1) / kSegments;
        draw->AddRectFilledMultiColor(ImVec2(origin.x + t0 * width, origin.y), ImVec2(origin.x + t1 * width, barMax.y),
                                      ToU32(gradient.Evaluate(t0)), ToU32(gradient.Evaluate(t1)),
                                      ToU32(gradient.Evaluate(t1)), ToU32(gradient.Evaluate(t0)));
    }

    ImGui::InvisibleButton("##markers", ImVec2(width, 34.0f));
    ImVec2 mouse = ImGui::GetIO().MousePos;
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID dragId = ImGui::GetID("##drag");
    int dragging = storage->GetInt(dragId, -1);
    int hovered = -1;
    for (int i = 0; i < (int)gradient.Keys.size(); i++) {
        float x = origin.x + gradient.Keys[i].Time * width;
        if (ImGui::IsItemHovered() && std::abs(mouse.x - x) < 6.0f && mouse.y > barMax.y)
            hovered = i;
        ImVec2 tip(x, barMax.y + 1.0f);
        ImU32 outline = i == hovered || i == dragging ? IM_COL32(255, 255, 255, 255) : IM_COL32(140, 140, 140, 255);
        draw->AddTriangleFilled(tip, ImVec2(x - 6.0f, tip.y + 10.0f), ImVec2(x + 6.0f, tip.y + 10.0f), outline);
        glm::vec4 c = gradient.Keys[i].Color;
        draw->AddRectFilled(ImVec2(x - 5.0f, tip.y + 10.0f), ImVec2(x + 5.0f, tip.y + 20.0f),
                            ToU32(glm::vec4(glm::vec3(c), 1.0f)));
        draw->AddRect(ImVec2(x - 5.0f, tip.y + 10.0f), ImVec2(x + 5.0f, tip.y + 20.0f), outline);
    }
    if (ImGui::IsItemActivated())
        storage->SetInt(dragId, dragging = hovered);
    if (dragging >= 0 && dragging < (int)gradient.Keys.size() && ImGui::IsItemActive()) {
        float lo = dragging > 0 ? gradient.Keys[dragging - 1].Time : 0.0f;
        float hi = dragging + 1 < (int)gradient.Keys.size() ? gradient.Keys[dragging + 1].Time : 1.0f;
        gradient.Keys[dragging].Time = std::clamp((mouse.x - origin.x) / width, lo, hi);
        changed = true;
    }
    if (!ImGui::IsItemActive())
        storage->SetInt(dragId, -1);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered < 0) {
        float t = std::clamp((mouse.x - origin.x) / width, 0.0f, 1.0f);
        gradient.Keys.push_back({t, gradient.Evaluate(t)});
        gradient.Sort();
        changed = true;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && hovered >= 0 && gradient.Keys.size() > 1) {
        gradient.Keys.erase(gradient.Keys.begin() + hovered);
        changed = true;
    }

    if (ImGui::TreeNode("Keys")) {
        for (int i = 0; i < (int)gradient.Keys.size(); i++) {
            ImGui::PushID(i);
            float lo = i > 0 ? gradient.Keys[i - 1].Time : 0.0f;
            float hi = i + 1 < (int)gradient.Keys.size() ? gradient.Keys[i + 1].Time : 1.0f;
            ImGui::SetNextItemWidth(70.0f);
            changed |=
                ImGui::DragFloat("##t", &gradient.Keys[i].Time, 0.005f, lo, hi, "t %.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            changed |= ImGui::ColorEdit4("##c", &gradient.Keys[i].Color.x,
                                         ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf |
                                             ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::SameLine();
            ImGui::Text("a %.2f", gradient.Keys[i].Color.a);
            ImGui::SameLine();
            if (gradient.Keys.size() > 1 && ImGui::SmallButton("x")) {
                gradient.Keys.erase(gradient.Keys.begin() + i);
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::SmallButton("+ Key")) {
            glm::vec4 last = gradient.Keys.back().Color;
            gradient.Keys.back().Time = std::min(gradient.Keys.back().Time, 0.99f);
            gradient.Keys.push_back({1.0f, last});
            changed = true;
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
    return changed;
}

bool RangeEditor(const char* label, float& min, float& max, float speed, float lowest = 0.0f, float highest = 1e6f) {
    return ImGui::DragFloatRange2(label, &min, &max, speed, lowest, highest, "%.3f", "%.3f",
                                  ImGuiSliderFlags_AlwaysClamp);
}

constexpr const char* kShapeNames[] = {"Point", "Cone", "Sphere", "Box"};
constexpr const char* kBlendNames[] = {"Alpha", "Additive"};
constexpr const char* kOrientationNames[] = {"Billboard", "Velocity Aligned", "Velocity Stretched"};
constexpr const char* kBackgroundNames[] = {"Dark", "Light", "Checker"};

ParticleLayer DefaultLayer() {
    ParticleLayer layer;
    layer.Name = "Layer";
    layer.Emission.BurstCount = 20;
    layer.Emission.SpeedMin = 1.0f;
    layer.Emission.SpeedMax = 2.0f;
    layer.Appearance.SizeMin = 0.15f;
    layer.Appearance.SizeMax = 0.25f;
    return layer;
}

} // namespace

ParticleEditor::ParticleEditor()
    : m_Preview(CreateScope<ParticleSystem>(4096)), m_Camera(50.0f, 16.0f / 9.0f, 0.05f, 200.0f) {}

ParticleEditor::~ParticleEditor() = default;

void ParticleEditor::SetAssetRoots(const std::string& runtimeRoot, const std::string& sourceRoot) {
    m_RuntimeRoot = runtimeRoot;
    m_SourceRoot = sourceRoot;
    m_LastScan = -1e9;
}

void ParticleEditor::Open(const std::string& path) {
    Ref<ParticleEffect> effect = ParticleLibrary::Load(path);
    if (!effect) {
        m_Status = "Can't load " + path;
        m_StatusTime = ImGui::GetTime();
        return;
    }
    m_Path = path;
    m_Effect = effect;
    m_SelectedLayer = 0;
    RestartPreview();
}

std::string ParticleEditor::RelativePath(const std::string& runtimePath) const {
    std::string prefix = m_RuntimeRoot + "/";
    if (runtimePath.rfind(prefix, 0) == 0)
        return runtimePath.substr(prefix.size());
    return runtimePath;
}

std::string ParticleEditor::SourcePath(const std::string& runtimePath) const {
    if (m_SourceRoot.empty())
        return {};
    return m_SourceRoot + "/" + RelativePath(runtimePath);
}

void ParticleEditor::ScanFiles() {
    m_Files.clear();
    m_Textures.clear();
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_RuntimeRoot + "/Particles", ec)) {
        std::string ext = entry.path().extension().string();
        if (ext == ".particle")
            m_Files.push_back(entry.path().generic_string());
        else if (ext == ".png")
            m_Textures.push_back(entry.path().generic_string());
    }
    std::sort(m_Files.begin(), m_Files.end());
    std::sort(m_Textures.begin(), m_Textures.end());
    m_LastScan = ImGui::GetTime();
}

bool ParticleEditor::SaveTo(const ParticleEffect& effect, const std::string& runtimePath) {
    bool ok = ParticleLibrary::Save(effect, runtimePath);
    std::string source = SourcePath(runtimePath);
    if (!source.empty())
        ok = ParticleLibrary::Save(effect, source) && ok;
    return ok;
}

void ParticleEditor::Save() {
    if (!m_Effect)
        return;
    m_Effect->Validate();
    bool ok = SaveTo(*m_Effect, m_Path);
    if (ok)
        m_DirtyPaths.erase(m_Path);
    m_Status = ok ? "Saved " + RelativePath(m_Path) : "Save failed - see log";
    m_StatusTime = ImGui::GetTime();
}

void ParticleEditor::Revert() {
    if (!m_Effect)
        return;
    // The source copy is authoritative: the runtime copy is only refreshed by a rebuild or a Save.
    std::string source = SourcePath(m_Path);
    std::ifstream file(!source.empty() && std::filesystem::exists(source) ? source : m_Path);
    nlohmann::json json = nlohmann::json::parse(file, nullptr, false);
    if (json.is_discarded()) {
        m_Status = "Revert failed - can't parse the file";
        m_StatusTime = ImGui::GetTime();
        return;
    }
    ParticleEffect fresh = ParticleLibrary::FromJson(json);
    fresh.SourcePath = m_Effect->SourcePath;
    fresh.Validate();
    *m_Effect = std::move(fresh);
    m_SelectedLayer = std::min(m_SelectedLayer, std::max((int)m_Effect->Layers.size() - 1, 0));
    m_DirtyPaths.erase(m_Path);
    m_Status = "Reverted " + RelativePath(m_Path);
    m_StatusTime = ImGui::GetTime();
    RestartPreview();
}

void ParticleEditor::MarkDirty() {
    m_DirtyPaths.insert(m_Path);
    if (m_RestartOnEdit)
        RestartPreview();
}

void ParticleEditor::RestartPreview() {
    m_Preview->Clear();
    m_Time = 0.0f;
    m_Started = false;
    m_Accumulators.assign(m_Effect ? m_Effect->Layers.size() : 0, 0.0f);
    m_PreviousOrigin = EmitterOrigin(0.0f);
}

glm::vec3 ParticleEditor::EmitterOrigin(float time) const {
    glm::vec3 origin(0.0f, 0.3f, 0.0f);
    if (m_MoveEmitter)
        origin += glm::vec3(std::cos(time * m_MoveSpeed), 0.0f, std::sin(time * m_MoveSpeed)) * m_MoveRadius;
    return origin;
}

void ParticleEditor::StepPreview(float dt) {
    if (!m_Effect || dt <= 0.0f)
        return;
    glm::vec3 direction = glm::dot(m_EmitDirection, m_EmitDirection) > 1e-6f ? glm::normalize(m_EmitDirection)
                                                                             : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 origin = EmitterOrigin(m_Time);
    glm::vec3 velocity = (origin - m_PreviousOrigin) / dt;
    m_PreviousOrigin = origin;

    if (!m_Started) {
        m_Preview->Play(m_Effect, origin, direction, velocity);
        m_Started = true;
    }
    if (m_Continuous) {
        m_Accumulators.resize(m_Effect->Layers.size(), 0.0f);
        for (uint32_t i = 0; i < m_Effect->Layers.size(); i++) {
            const ParticleLayer& layer = m_Effect->Layers[i];
            if (!layer.Enabled || layer.Emission.Rate <= 0.0f)
                continue;
            m_Accumulators[i] += layer.Emission.Rate * dt;
            auto n = (uint32_t)m_Accumulators[i];
            if (n > 0) {
                m_Accumulators[i] -= (float)n;
                m_Preview->Emit(m_Effect, i, origin, direction, velocity, n);
            }
        }
    }
    m_Preview->Simulate(dt);
    m_Time += dt;
    if (m_Loop && m_Time >= std::max(m_Effect->Duration, 0.05f)) {
        // Loops re-fire the burst without clearing, so lingering particles overlap like repeated shots in game.
        m_Time = 0.0f;
        m_Started = false;
        m_PreviousOrigin = EmitterOrigin(0.0f);
    }
}

void ParticleEditor::RenderPreview(uint32_t width, uint32_t height) {
    if (!m_Target)
        m_Target = CreateScope<RenderTarget>(width, height);
    m_Target->Resize(width, height);

    glm::vec3 offset(std::cos(m_Pitch) * std::sin(m_Yaw), std::sin(m_Pitch), std::cos(m_Pitch) * std::cos(m_Yaw));
    glm::vec3 position = m_Focus + offset * m_Distance;
    m_Camera.SetAspect((float)width / (float)height);
    m_Camera.SetPosition(position);
    m_Camera.SetOrientation(glm::quatLookAt(glm::normalize(m_Focus - position), glm::vec3(0.0f, 1.0f, 0.0f)));

    m_Target->Begin();
    glm::vec3 clear =
        m_Background == Background::Light ? glm::vec3(0.72f, 0.74f, 0.78f) : glm::vec3(0.07f, 0.07f, 0.08f);
    glClearColor(clear.r, clear.g, clear.b, 1.0f);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Renderer::BeginScene(m_Camera);
    if (m_ShowGround) {
        std::vector<DebugTriangle> ground;
        constexpr int kTiles = 12;
        constexpr float kTile = 0.5f;
        for (int z = -kTiles; z < kTiles; z++) {
            for (int x = -kTiles; x < kTiles; x++) {
                bool odd = ((x + z) & 1) != 0;
                glm::vec3 color = m_Background == Background::Checker
                                      ? (odd ? glm::vec3(0.55f) : glm::vec3(0.35f))
                                      : (odd ? glm::vec3(0.22f, 0.23f, 0.25f) : glm::vec3(0.18f, 0.19f, 0.21f));
                glm::vec3 a(x * kTile, 0.0f, z * kTile), b((x + 1) * kTile, 0.0f, z * kTile);
                glm::vec3 c((x + 1) * kTile, 0.0f, (z + 1) * kTile), d(x * kTile, 0.0f, (z + 1) * kTile);
                ground.push_back({a, c, b, color});
                ground.push_back({a, d, c, color});
            }
        }
        Renderer::SubmitGameplayTriangles(ground);
    }
    if (m_ShowEmitter) {
        glm::vec3 origin = EmitterOrigin(m_Time);
        glm::vec3 axis = glm::dot(m_EmitDirection, m_EmitDirection) > 1e-6f ? glm::normalize(m_EmitDirection)
                                                                            : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 color(0.2f, 1.0f, 0.4f);
        constexpr float kCross = 0.08f;
        Renderer::SubmitGameplayLines({{origin - glm::vec3(kCross, 0, 0), origin + glm::vec3(kCross, 0, 0), color},
                                       {origin - glm::vec3(0, kCross, 0), origin + glm::vec3(0, kCross, 0), color},
                                       {origin - glm::vec3(0, 0, kCross), origin + glm::vec3(0, 0, kCross), color},
                                       {origin, origin + axis * 0.5f, color}});
    }
    m_Preview->SubmitDebugLines(m_Debug);
    Renderer::EndScene();
    m_Preview->Render(m_Camera);
    m_Target->End();
}

void ParticleEditor::DrawImGui() {
    if (ImGui::GetTime() - m_LastScan > 2.0)
        ScanFiles();

    DrawToolbar();
    ImGui::Separator();

    float sideWidth = std::min(230.0f, ImGui::GetContentRegionAvail().x * 0.3f);
    ImGui::BeginChild("##side", ImVec2(sideWidth, 0.0f), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
    DrawBrowser();
    ImGui::Spacing();
    DrawLayerList();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##main", ImVec2(0.0f, 0.0f));
    DrawPreview(m_PreviewHeight);
    ImGui::BeginChild("##properties", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    DrawProperties();
    ImGui::EndChild();
    ImGui::EndChild();
}

void ParticleEditor::DrawToolbar() {
    bool hasEffect = m_Effect != nullptr;
    bool dirty = hasEffect && m_DirtyPaths.count(m_Path) > 0;

    if (ImGui::Button("New"))
        ImGui::OpenPopup("New Effect");
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasEffect);
    if (ImGui::Button("Duplicate"))
        ImGui::OpenPopup("Duplicate Effect");
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
        ImGui::OpenPopup("Delete Effect");
    ImGui::SameLine();
    ImGui::BeginDisabled(!dirty);
    if (ImGui::Button("Save") || (dirty && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                  ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)))
        Save();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Revert"))
        Revert();
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (hasEffect)
        ImGui::Text("%s%s", RelativePath(m_Path).c_str(), dirty ? " *" : "");
    else
        ImGui::TextDisabled("No effect open - pick one on the left");
    if (!m_Status.empty() && ImGui::GetTime() - m_StatusTime < 4.0) {
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", m_Status.c_str());
    }

    DrawPathPopup("New Effect", false);
    DrawPathPopup("Duplicate Effect", true);
    DrawDeletePopup();
}

void ParticleEditor::DrawPathPopup(const char* id, bool duplicate) {
    if (ImGui::IsPopupOpen(id) && m_PathBuffer[0] == '\0') {
        std::string seed = duplicate && m_Effect ? RelativePath(m_Path) : "Particles/NewEffect.particle";
        if (duplicate)
            seed.insert(seed.size() - std::string(".particle").size(), "_Copy");
        std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", seed.c_str());
    }
    if (!ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::Text("Path under %s/", m_RuntimeRoot.c_str());
    ImGui::SetNextItemWidth(360.0f);
    bool enter = ImGui::InputText("##path", m_PathBuffer, sizeof(m_PathBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    std::string relative = m_PathBuffer;
    if (relative.size() < 9 || relative.substr(relative.size() - 9) != ".particle")
        relative += ".particle";
    std::string path = m_RuntimeRoot + "/" + relative;
    bool exists = std::filesystem::exists(path);
    if (exists)
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "A file already exists there");

    ImGui::BeginDisabled(exists || relative == ".particle");
    if (ImGui::Button("Create") || (enter && !exists)) {
        ParticleEffect effect;
        if (duplicate && m_Effect)
            effect = *m_Effect;
        else
            effect.Layers.push_back(DefaultLayer());
        effect.Name = std::filesystem::path(relative).stem().string();
        effect.SourcePath = path;
        if (SaveTo(effect, path)) {
            ScanFiles();
            Open(path);
        }
        m_PathBuffer[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        m_PathBuffer[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ParticleEditor::DrawDeletePopup() {
    if (!ImGui::BeginPopupModal("Delete Effect", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    ImGui::Text("Delete %s from disk?", RelativePath(m_Path).c_str());
    ImGui::TextDisabled("Removes both the runtime and source copies. Anything still using it keeps its copy.");
    if (ImGui::Button("Delete")) {
        std::error_code ec;
        std::filesystem::remove(m_Path, ec);
        std::string source = SourcePath(m_Path);
        if (!source.empty())
            std::filesystem::remove(source, ec);
        ParticleLibrary::Forget(m_Path);
        m_DirtyPaths.erase(m_Path);
        m_Status = "Deleted " + RelativePath(m_Path);
        m_StatusTime = ImGui::GetTime();
        m_Path.clear();
        m_Effect = nullptr;
        RestartPreview();
        ScanFiles();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void ParticleEditor::DrawBrowser() {
    ImGui::SeparatorText("Effects");
    ImGui::BeginChild("##files", ImVec2(0.0f, 220.0f));
    std::string prefix = m_RuntimeRoot + "/Particles/";
    for (const std::string& file : m_Files) {
        std::string label = file.rfind(prefix, 0) == 0 ? file.substr(prefix.size()) : file;
        if (m_DirtyPaths.count(file))
            label += " *";
        if (ImGui::Selectable(label.c_str(), file == m_Path) && file != m_Path)
            Open(file);
    }
    ImGui::EndChild();
}

void ParticleEditor::DrawLayerList() {
    ImGui::SeparatorText("Layers");
    if (!m_Effect) {
        ImGui::TextDisabled("No effect open");
        return;
    }
    auto& layers = m_Effect->Layers;
    bool structural = false;
    for (int i = 0; i < (int)layers.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::Checkbox("##enabled", &layers[i].Enabled))
            MarkDirty();
        ImGui::SameLine();
        std::string label = layers[i].Name.empty() ? "(unnamed)" : layers[i].Name;
        if (ImGui::Selectable(label.c_str(), i == m_SelectedLayer))
            m_SelectedLayer = i;
        ImGui::PopID();
    }

    if (ImGui::SmallButton("Add")) {
        layers.push_back(DefaultLayer());
        m_SelectedLayer = (int)layers.size() - 1;
        structural = true;
    }
    bool hasSelection = m_SelectedLayer >= 0 && m_SelectedLayer < (int)layers.size();
    ImGui::BeginDisabled(!hasSelection);
    ImGui::SameLine();
    if (ImGui::SmallButton("Dup") && hasSelection) {
        ParticleLayer copy = layers[m_SelectedLayer];
        copy.Name += " Copy";
        layers.insert(layers.begin() + m_SelectedLayer + 1, copy);
        m_SelectedLayer++;
        structural = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove") && hasSelection) {
        layers.erase(layers.begin() + m_SelectedLayer);
        m_SelectedLayer = std::max(0, std::min(m_SelectedLayer, (int)layers.size() - 1));
        structural = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##up", ImGuiDir_Up) && hasSelection && m_SelectedLayer > 0) {
        std::swap(layers[m_SelectedLayer], layers[m_SelectedLayer - 1]);
        m_SelectedLayer--;
        structural = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##down", ImGuiDir_Down) && hasSelection && m_SelectedLayer + 1 < (int)layers.size()) {
        std::swap(layers[m_SelectedLayer], layers[m_SelectedLayer + 1]);
        m_SelectedLayer++;
        structural = true;
    }
    ImGui::EndDisabled();
    if (ImGui::SmallButton("Import layer..."))
        ImGui::OpenPopup("Import Layer");
    DrawImportPopup();

    if (structural) {
        MarkDirty();
        RestartPreview();
    }
}

void ParticleEditor::DrawImportPopup() {
    if (!ImGui::BeginPopup("Import Layer"))
        return;
    ImGui::TextUnformatted("Copy a layer from another effect");
    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::BeginCombo("Effect", m_ImportPath.empty() ? "(pick one)" : RelativePath(m_ImportPath).c_str())) {
        for (const std::string& file : m_Files) {
            if (ImGui::Selectable(RelativePath(file).c_str(), file == m_ImportPath))
                m_ImportPath = file;
        }
        ImGui::EndCombo();
    }
    if (!m_ImportPath.empty()) {
        if (Ref<ParticleEffect> source = ParticleLibrary::Load(m_ImportPath)) {
            for (size_t i = 0; i < source->Layers.size(); i++) {
                ImGui::PushID((int)i);
                if (ImGui::Selectable(source->Layers[i].Name.c_str())) {
                    ParticleLayer copy = source->Layers[i];
                    m_Effect->Layers.push_back(copy);
                    m_SelectedLayer = (int)m_Effect->Layers.size() - 1;
                    MarkDirty();
                    RestartPreview();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
            }
        }
    }
    ImGui::EndPopup();
}

void ParticleEditor::DrawPreview(float height) {
    float width = ImGui::GetContentRegionAvail().x;
    height = std::max(height, 120.0f);

    if (m_Effect && m_Playing)
        StepPreview(std::min(ImGui::GetIO().DeltaTime, 0.1f) * m_PlaybackSpeed);

    ImVec2 min = ImGui::GetCursorScreenPos();
    ImVec2 size(std::max(width, 32.0f), height);
    ImGui::InvisibleButton("##preview", size,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsItemActive()) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_Yaw -= io.MouseDelta.x * 0.01f;
            m_Pitch = std::clamp(m_Pitch + io.MouseDelta.y * 0.01f, -1.5f, 1.5f);
        } else {
            glm::vec3 offset(std::cos(m_Pitch) * std::sin(m_Yaw), std::sin(m_Pitch),
                             std::cos(m_Pitch) * std::cos(m_Yaw));
            glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), offset));
            glm::vec3 up = glm::cross(offset, right);
            float scale = m_Distance * 0.002f;
            m_Focus += (-right * io.MouseDelta.x + up * io.MouseDelta.y) * scale;
        }
    }
    if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        m_Distance = std::clamp(m_Distance * std::pow(0.88f, io.MouseWheel), 0.3f, 60.0f);

    RenderPreview((uint32_t)size.x, (uint32_t)size.y);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 max(min.x + size.x, min.y + size.y);
    draw->AddImage(ToImTexture(m_Target->GetColorTexture()), min, max, kUvTop, kUvBottom);
    const ParticleSystem::Stats& stats = m_Preview->GetStats();
    char overlay[160];
    std::snprintf(overlay, sizeof(overlay), "t %.2fs   alive %u   draws %u   alpha %u   additive %u", m_Time,
                  stats.Alive, stats.DrawCalls, stats.AlphaParticles, stats.AdditiveParticles);
    draw->AddText(ImVec2(min.x + 6.0f, min.y + 4.0f), IM_COL32(230, 230, 230, 220), overlay);
    draw->AddText(ImVec2(min.x + 6.0f, max.y - 18.0f), IM_COL32(180, 180, 180, 160),
                  "LMB orbit  RMB/MMB pan  wheel zoom");

    // Splitter between the preview and the properties below.
    ImGui::InvisibleButton("##split", ImVec2(size.x, 5.0f));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive())
        m_PreviewHeight = std::clamp(m_PreviewHeight + io.MouseDelta.y, 120.0f, 1400.0f);

    // Playback
    if (ImGui::Button(m_Playing ? "Pause" : "Play"))
        m_Playing = !m_Playing;
    ImGui::SameLine();
    if (ImGui::Button("Restart"))
        RestartPreview();
    ImGui::SameLine();
    if (ImGui::Button("Burst") && m_Effect) {
        glm::vec3 direction = glm::dot(m_EmitDirection, m_EmitDirection) > 1e-6f ? glm::normalize(m_EmitDirection)
                                                                                 : glm::vec3(0.0f, 1.0f, 0.0f);
        m_Preview->Play(m_Effect, EmitterOrigin(m_Time), direction);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &m_Loop);
    ImGui::SameLine();
    ImGui::Checkbox("Continuous", &m_Continuous);
    ImGui::SameLine();
    ImGui::Checkbox("Restart on edit", &m_RestartOnEdit);

    if (m_Effect) {
        float duration = std::max(m_Effect->Duration, 0.05f);
        float scrub = m_Time;
        ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - 200.0f, 80.0f));
        if (ImGui::SliderFloat("##time", &scrub, 0.0f, duration, "%.2f s")) {
            // Scrubbing replays from the start in fixed steps, so the result matches normal playback.
            RestartPreview();
            bool loop = m_Loop;
            m_Loop = false;
            while (m_Time + kStep <= scrub)
                StepPreview(kStep);
            m_Loop = loop;
            m_Playing = false;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("Speed", &m_PlaybackSpeed, 0.05f, 2.0f, "%.2fx");
    }

    if (ImGui::TreeNode("Preview Settings")) {
        EnumCombo("Background", m_Background, kBackgroundNames);
        ImGui::Checkbox("Ground", &m_ShowGround);
        ImGui::SameLine();
        ImGui::Checkbox("Emitter", &m_ShowEmitter);
        ImGui::SameLine();
        ImGui::Checkbox("Velocity", &m_Debug.Velocity);
        ImGui::SameLine();
        ImGui::Checkbox("Centers", &m_Debug.Centers);
        ImGui::DragFloat3("Emit direction", &m_EmitDirection.x, 0.02f, -1.0f, 1.0f);
        ImGui::Checkbox("Moving emitter", &m_MoveEmitter);
        if (m_MoveEmitter) {
            ImGui::DragFloat("Radius", &m_MoveRadius, 0.02f, 0.0f, 10.0f);
            ImGui::DragFloat("Angular speed", &m_MoveSpeed, 0.05f, -20.0f, 20.0f);
        }
        if (ImGui::Button("Reset camera")) {
            m_Yaw = 0.7f;
            m_Pitch = 0.3f;
            m_Distance = 4.0f;
            m_Focus = glm::vec3(0.0f, 0.6f, 0.0f);
        }
        ImGui::Text("Pool %u / %u (peak %u, dropped %llu)", stats.Alive, stats.Capacity, stats.PeakAlive,
                    (unsigned long long)stats.Dropped);
        ImGui::TreePop();
    }
}

void ParticleEditor::DrawProperties() {
    if (!m_Effect) {
        ImGui::TextDisabled("Open or create an effect to edit it.");
        return;
    }
    bool changed = false;
    ImGui::PushItemWidth(-140.0f);

    changed |= InputString("Effect name", m_Effect->Name);
    changed |= ImGui::DragFloat("Duration", &m_Effect->Duration, 0.01f, 0.05f, 60.0f, "%.2f s");

    if (m_SelectedLayer < 0 || m_SelectedLayer >= (int)m_Effect->Layers.size()) {
        ImGui::PopItemWidth();
        if (changed)
            MarkDirty();
        ImGui::TextDisabled("No layer selected.");
        return;
    }
    ParticleLayer& layer = m_Effect->Layers[m_SelectedLayer];
    ImGui::SeparatorText("Layer");
    changed |= InputString("Name", layer.Name);
    changed |= ImGui::Checkbox("Enabled", &layer.Enabled);

    ParticleEmission& em = layer.Emission;
    if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat("Rate (/s)", &em.Rate, 0.5f, 0.0f, 5000.0f);
        int burst = (int)em.BurstCount;
        if (ImGui::DragInt("Burst count", &burst, 0.2f, 0, 2000)) {
            em.BurstCount = (uint32_t)std::max(burst, 0);
            changed = true;
        }
        changed |= ImGui::DragFloat("Start delay", &em.StartDelay, 0.005f, 0.0f, 10.0f, "%.3f s");
        changed |= EnumCombo("Shape", em.Shape, kShapeNames);
        if (em.Shape == EmitShape::Cone)
            changed |= ImGui::SliderFloat("Cone angle", &em.ConeAngleDegrees, 0.0f, 180.0f, "%.1f deg");
        if (em.Shape == EmitShape::Box)
            changed |= ImGui::DragFloat3("Box half extents", &em.BoxHalfExtents.x, 0.01f, 0.0f, 100.0f);
        changed |= ImGui::DragFloat3("Local offset", &em.LocalOffset.x, 0.01f);
    }
    if (ImGui::CollapsingHeader("Lifetime & Speed", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= RangeEditor("Lifetime (s)", em.LifetimeMin, em.LifetimeMax, 0.005f, 0.001f, 60.0f);
        changed |= RangeEditor("Speed", em.SpeedMin, em.SpeedMax, 0.02f, 0.0f, 1000.0f);
        changed |= ImGui::SliderFloat("Inherit velocity", &em.InheritVelocity, 0.0f, 1.0f);
        changed |= ImGui::DragFloat3("Added velocity", &em.AddedVelocity.x, 0.02f);
    }
    if (ImGui::CollapsingHeader("Motion")) {
        ParticleMotion& motion = layer.Motion;
        changed |= ImGui::DragFloat3("Gravity", &motion.Gravity.x, 0.05f);
        changed |= ImGui::DragFloat("Drag", &motion.Drag, 0.01f, 0.0f, 50.0f);
        changed |= ImGui::DragFloat("Turbulence", &motion.TurbulenceStrength, 0.02f, 0.0f, 100.0f);
        if (motion.TurbulenceStrength > 0.0f) {
            changed |= ImGui::DragFloat("Turb. frequency", &motion.TurbulenceFrequency, 0.01f, 0.0f, 20.0f);
            changed |= ImGui::DragFloat("Turb. scroll", &motion.TurbulenceScroll, 0.01f, 0.0f, 20.0f);
        }
    }

    ParticleAppearance& look = layer.Appearance;
    if (ImGui::CollapsingHeader("Size & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= RangeEditor("Size", look.SizeMin, look.SizeMax, 0.002f, 0.0f, 100.0f);
        changed |= CurveEditor("Size over life", look.SizeOverLife, 0.0f, 2.0f);
        changed |= GradientEditor("Color over life", look.ColorOverLife);
        changed |= CurveEditor("Alpha over life", look.AlphaOverLife, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("Rotation & Orientation")) {
        changed |= ImGui::SliderFloat("Rotation jitter", &look.StartRotationJitter, 0.0f, 3.1416f, "%.2f rad");
        changed |=
            RangeEditor("Angular velocity", look.AngularVelocityMin, look.AngularVelocityMax, 0.02f, -50.0f, 50.0f);
        changed |= EnumCombo("Orientation", look.Orientation, kOrientationNames);
        if (look.Orientation != ParticleOrientation::Billboard)
            changed |= ImGui::DragFloat("Aspect", &look.Aspect, 0.02f, 0.01f, 50.0f);
        if (look.Orientation == ParticleOrientation::VelocityStretched)
            changed |= ImGui::DragFloat("Stretch factor", &look.StretchFactor, 0.001f, 0.0f, 10.0f, "%.4f");
    }

    ParticleMaterial& mat = layer.Material;
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::string current = mat.TexturePath.empty() ? "(built-in soft sprite)" : mat.TexturePath;
        if (ImGui::BeginCombo("Texture", current.c_str())) {
            if (ImGui::Selectable("(built-in soft sprite)", mat.TexturePath.empty())) {
                mat.TexturePath.clear();
                changed = true;
            }
            for (const std::string& texture : m_Textures) {
                if (ImGui::Selectable(texture.c_str(), texture == mat.TexturePath)) {
                    mat.TexturePath = texture;
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    if (Ref<Texture> tex = AssetManager::GetTexture(texture)) {
                        ImGui::BeginTooltip();
                        ImGui::Image(ToImTexture(tex->GetID()), ImVec2(96.0f, 96.0f), kUvTop, kUvBottom);
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::EndCombo();
        }
        changed |= ImGui::DragFloat4("Atlas rect", &mat.AtlasRect.x, 0.005f, 0.0f, 1.0f);
        changed |= EnumCombo("Blend", mat.Blend, kBlendNames);
        changed |= ImGui::DragFloat("Emissive", &mat.EmissiveStrength, 0.02f, 0.0f, 50.0f);

        changed |= ImGui::Checkbox("Soft particles", &mat.SoftParticles);
        if (mat.SoftParticles)
            changed |= ImGui::DragFloat("Soft distance", &mat.SoftDistance, 0.01f, 0.01f, 10.0f);
        changed |= ImGui::Checkbox("Erosion", &mat.Erosion);
        if (mat.Erosion) {
            changed |= CurveEditor("Erosion over life", mat.ErosionOverLife, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Erosion softness", &mat.ErosionSoftness, 0.001f, 1.0f);
        }
        changed |= ImGui::Checkbox("Distortion", &mat.Distortion);
        if (mat.Distortion) {
            changed |= ImGui::DragFloat("Distortion strength", &mat.DistortionStrength, 0.001f, 0.0f, 0.5f, "%.3f");
            changed |= ImGui::DragFloat("Distortion scale", &mat.DistortionScale, 0.01f, 0.01f, 20.0f);
            changed |= ImGui::DragFloat("Distortion scroll", &mat.DistortionScroll, 0.01f, 0.0f, 20.0f);
        }
    }

    ParticleFlipbook& fb = mat.Flipbook;
    if (ImGui::CollapsingHeader("Flipbook")) {
        changed |= ImGui::Checkbox("Flipbook enabled", &fb.Enabled);
        int grid[2] = {(int)fb.Columns, (int)fb.Rows};
        if (ImGui::DragInt2("Columns / rows", grid, 0.1f, 1, 32)) {
            fb.Columns = (uint32_t)std::max(grid[0], 1);
            fb.Rows = (uint32_t)std::max(grid[1], 1);
            changed = true;
        }
        int frames = (int)fb.FrameCount;
        if (ImGui::DragInt("Frame count", &frames, 0.1f, 1, (int)(fb.Columns * fb.Rows))) {
            fb.FrameCount = (uint32_t)std::max(frames, 1);
            changed = true;
        }
        changed |= ImGui::DragFloat("FPS (0 = over life)", &fb.FramesPerSecond, 0.1f, 0.0f, 120.0f);
        changed |= ImGui::Checkbox("Loop", &fb.Loop);
        ImGui::SameLine();
        changed |= ImGui::Checkbox("Random start frame", &fb.RandomStartFrame);
    }
    ImGui::PopItemWidth();

    // Texture with the atlas rect and flipbook grid on top, highlighting the frame the preview is showing.
    if (!mat.TexturePath.empty()) {
        if (Ref<Texture> tex = AssetManager::GetTexture(mat.TexturePath)) {
            ImGui::SeparatorText("Texture");
            float side = std::min(ImGui::GetContentRegionAvail().x, 192.0f);
            ImVec2 min = ImGui::GetCursorScreenPos();
            ImVec2 max(min.x + side, min.y + side);
            ImDrawList* draw = ImGui::GetWindowDrawList();
            DrawChecker(draw, min, max, 8.0f);
            ImGui::Image(ToImTexture(tex->GetID()), ImVec2(side, side), kUvTop, kUvBottom);
            ImVec2 rectMin(min.x + mat.AtlasRect.x * side, min.y + mat.AtlasRect.y * side);
            ImVec2 rectSize(mat.AtlasRect.z * side, mat.AtlasRect.w * side);
            draw->AddRect(rectMin, ImVec2(rectMin.x + rectSize.x, rectMin.y + rectSize.y), IM_COL32(150, 205, 64, 255));
            if (fb.Enabled && fb.Columns * fb.Rows > 1) {
                float cw = rectSize.x / (float)fb.Columns, ch = rectSize.y / (float)fb.Rows;
                for (uint32_t c = 1; c < fb.Columns; c++)
                    draw->AddLine(ImVec2(rectMin.x + c * cw, rectMin.y),
                                  ImVec2(rectMin.x + c * cw, rectMin.y + rectSize.y), IM_COL32(150, 205, 64, 120));
                for (uint32_t r = 1; r < fb.Rows; r++)
                    draw->AddLine(ImVec2(rectMin.x, rectMin.y + r * ch),
                                  ImVec2(rectMin.x + rectSize.x, rectMin.y + r * ch), IM_COL32(150, 205, 64, 120));
                float life = std::max(0.5f * (em.LifetimeMin + em.LifetimeMax), 0.001f);
                float age = std::fmod(m_Time, life);
                uint32_t frame = fb.FramesPerSecond > 0.0f ? (uint32_t)(age * fb.FramesPerSecond)
                                                           : (uint32_t)(age / life * (float)fb.FrameCount);
                frame =
                    fb.Loop || fb.FramesPerSecond <= 0.0f ? frame % fb.FrameCount : std::min(frame, fb.FrameCount - 1);
                float fx = rectMin.x + (float)(frame % fb.Columns) * cw;
                float fy = rectMin.y + (float)(frame / fb.Columns) * ch;
                draw->AddRect(ImVec2(fx, fy), ImVec2(fx + cw, fy + ch), IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
            }
            ImGui::TextDisabled("%ux%u", tex->GetWidth(), tex->GetHeight());
        }
    }

    if (changed) {
        layer.Validate();
        MarkDirty();
    }
}

} // namespace Wankel
