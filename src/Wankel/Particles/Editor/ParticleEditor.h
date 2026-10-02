#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/ParticleSystem.h"
#include "Wankel/Renderer/Camera.h"

#include <glm/glm.hpp>

#include <set>
#include <string>
#include <vector>

namespace Wankel {

class RenderTarget;
struct ParticleEffect;

// ImGui editor for .particle assets: effect browser, layer list, property editing and a live preview.
// It edits the ParticleLibrary-cached effect itself, so changes show up in game immediately.
// Draws into whatever window the host has begun, so a client can dock it in its own panel framework.
class ParticleEditor {
public:
    ParticleEditor();
    ~ParticleEditor();

    ParticleEditor(const ParticleEditor&) = delete;
    ParticleEditor& operator=(const ParticleEditor&) = delete;

    // runtimeRoot is where the game loads from (e.g. "Assets"); saves also go to sourceRoot when it's set.
    void SetAssetRoots(const std::string& runtimeRoot, const std::string& sourceRoot);

    // Opens an effect by its runtime path (e.g. "Assets/Particles/Weapons/MuzzleFlash.particle").
    void Open(const std::string& path);
    const std::string& GetOpenPath() const { return m_Path; }

    // Draws the editor into the current ImGui window and advances/renders the preview.
    void DrawImGui();

private:
    enum class Background : uint8_t { Dark, Light, Checker };

    void ScanFiles();
    std::string RelativePath(const std::string& runtimePath) const;
    std::string SourcePath(const std::string& runtimePath) const;
    bool SaveTo(const ParticleEffect& effect, const std::string& runtimePath);
    void Save();
    void Revert();
    void MarkDirty();

    void DrawToolbar();
    void DrawBrowser();
    void DrawLayerList();
    void DrawPreview(float height);
    void DrawProperties();
    void DrawPathPopup(const char* id, bool duplicate);
    void DrawDeletePopup();
    void DrawImportPopup();

    void RestartPreview();
    void StepPreview(float dt);
    glm::vec3 EmitterOrigin(float time) const;
    void RenderPreview(uint32_t width, uint32_t height);

    std::string m_RuntimeRoot = "Assets";
    std::string m_SourceRoot;

    std::string m_Path;
    Ref<ParticleEffect> m_Effect;
    int m_SelectedLayer = 0;
    std::set<std::string> m_DirtyPaths;

    std::vector<std::string> m_Files;
    std::vector<std::string> m_Textures;
    double m_LastScan = -1e9;
    char m_PathBuffer[256] = {};
    std::string m_ImportPath;
    std::string m_Status;
    double m_StatusTime = -1e9;

    Scope<ParticleSystem> m_Preview;
    Scope<RenderTarget> m_Target;
    Camera m_Camera;
    float m_Yaw = 0.7f, m_Pitch = 0.3f, m_Distance = 4.0f;
    glm::vec3 m_Focus {0.0f, 0.6f, 0.0f};

    bool m_Playing = true;
    bool m_Loop = true;
    bool m_Continuous = true;
    bool m_RestartOnEdit = true;
    float m_PlaybackSpeed = 1.0f;
    float m_Time = 0.0f;
    bool m_Started = false;
    std::vector<float> m_Accumulators;

    glm::vec3 m_EmitDirection {0.0f, 1.0f, 0.0f};
    bool m_MoveEmitter = false;
    float m_MoveRadius = 1.0f;
    float m_MoveSpeed = 2.0f;
    glm::vec3 m_PreviousOrigin {0.0f};

    Background m_Background = Background::Dark;
    bool m_ShowGround = true;
    bool m_ShowEmitter = true;
    ParticleSystem::DebugOptions m_Debug;
    float m_PreviewHeight = 320.0f;
};

} // namespace Wankel
