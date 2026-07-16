#pragma once

#include "Systems/PlayerInputSystem.h"

#include <Wankel/Core/Layer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/ECS/Scene.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/ECS/Components.h>
#include <memory>

namespace Wankel {

class VertexArray;
class VertexBuffer;
class Mesh;
class Font;
class AudioClip;

class SandboxLayer : public Layer {
public:
    SandboxLayer();
    virtual void OnUpdate() override;
    virtual void OnEvent(Event& e) override;
    virtual void OnImGuiRender() override;

private:
    Ref<Mesh> m_ShipMesh;
    Ref<Mesh> m_ShipMeshMirrored;
    Ref<Mesh> m_PlayerHeadMesh;
    Ref<Mesh> m_PlayerLegMesh;
    Ref<Mesh> m_PlayerLegMeshMirrored;
    Ref<Mesh> m_EnemyBodyMesh;
    Ref<Mesh> m_EnemyLegMesh;
    Ref<Mesh> m_EnemyLegMeshMirrored;
    Ref<Mesh> m_GunMesh;
    Ref<Mesh> m_CubeMesh;
    Ref<Mesh> m_BoxMesh;
    Ref<Shader> m_Shader;
    Ref<Font> m_TitleFont;
    Ref<AudioClip> m_ClickMissBeep;
    Ref<AudioClip> m_ClickHitBeep;
    entt::entity m_SelectedAnimEntity = entt::null;

    float m_ChunkSize = 200.0f; // ±100m cutoff
    int m_RepeatN = 1;          // 1 = 3x3 grid

    Camera m_RenderCamera;
    Scene m_Scene;
    PlayerInputSystem m_PlayerInputSystem;

    float m_LastFrame = 0.0f;

    // IMGUI / DEBUG
    FogSettings m_Fog;
    LightSettings m_Light;
    bool m_GameFocused = true;
};

} // namespace Wankel
