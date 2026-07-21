#include "SandboxLayer.h"
#include "Debug/DebugOverlay.h"
#include "Debug/ColliderDebugDraw.h"
#include "Debug/AnimationDebugPanel.h"
#include "Entities/PlayerFactory.h"
#include "Entities/EnemyFactory.h"
#include "Entities/WorldFactory.h"

#include <Wankel/Assets/AssetManager.h>
#include <Wankel/Core/Application.h>
#include <Wankel/Core/Time.h>
#include <Wankel/Core/Events/Event.h>
#include "Wankel/Core/Input.h"
#include "Wankel/Core/ControllerInput.h"
#include "Wankel/Core/KeyCodes.h"
#include <Wankel/ECS/Components.h>
#include <Wankel/Renderer/Renderer.h>
#include <Wankel/Renderer/Shader.h>
#include <Wankel/Renderer/Font.h>
#include <Wankel/Audio/AudioClip.h>
#include <Wankel/Audio/AudioSystem.h>
#include <Wankel/Core/Window.h>
#include <Wankel/Renderer/DebugDraw.h>
#include <Wankel/Physics/Raycast/Ray.h>
#include <Wankel/Physics/Raycast/Raycast.h>
#include <Wankel/Physics/Raycast/RaycastHit.h>

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>


namespace Wankel {

Ray CreateCameraRay(const Camera& cam, glm::vec3 origin) {
    Ray ray;
    ray.Origin = origin; //cam.GetPosition() + glm::normalize(cam.GetForward()) * 1.6f;
    ray.Direction = glm::normalize(cam.GetForward());
    return ray;
}

static float RandomFloat() {
    static std::random_device rd;  // seed
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    return dist(gen);
}

SandboxLayer::SandboxLayer() : Layer("Cube") {
    // Shader
    m_Shader = AssetManager::GetShader("shaders/cube.vert", "shaders/cube.frag");

    // HUD title text
    m_TitleFont = AssetManager::GetFont("Assets/Fonts/Orbitron-VariableFont_wght.ttf", 32.0f);

    // Click-test beeps: low tone on a miss, higher tone on a block hit.
    m_ClickMissBeep = AudioClip::CreateTone(220.0f, 0.12f);
    m_ClickHitBeep = AudioClip::CreateTone(880.0f, 0.12f);

    PlayerFactory::Create(m_Scene);
    EnemyFactory::Create(m_Scene);
    WorldFactory::Create(m_Scene);

    // DEFAULT FOG
    m_Fog.Color = {0.78f, 0.98f, 1.0f};
    m_Fog.Density = 0.008f;

    // Lock mouse initially
    Application::Get().GetWindow().SetCursorMode(CursorMode::Disabled);
}


void SandboxLayer::OnUpdate() {
    float time = Time::GetTime();
    float dt = time - m_LastFrame;
    m_LastFrame = time;

    DebugOverlay::PushFrameTime(dt);

    // ADD ESC to switch to ImGui
    static bool escPressedLastFrame = false;
    bool escPressed = Input::IsKeyPressed(Key::Escape);
    if (escPressed && !escPressedLastFrame) {
        m_GameFocused = !m_GameFocused;
        auto& window = Application::Get().GetWindow();
        window.SetCursorMode(m_GameFocused ? CursorMode::Disabled : CursorMode::Normal);
    }
    escPressedLastFrame = escPressed;

    m_PlayerInputSystem.Update(m_Scene, dt, m_GameFocused);

    m_Scene.OnUpdate(dt, m_RenderCamera);

    auto playerView = m_Scene.Registry().view<Transform, PlayerController>();

    for (auto entity : playerView) {
        auto& playerTransform = playerView.get<Transform>(entity);

        // Wrap player if they chunk border limit
        glm::vec3 pos = playerTransform.LocalPosition;
        pos = glm::mod(pos + m_ChunkSize * 0.5f, m_ChunkSize) - m_ChunkSize * 0.5f;
        playerTransform.LocalPosition = pos;
    }


    Renderer::SetFog(m_Fog);
    Renderer::SetLight(m_Light);

    glm::vec3 camPos = m_RenderCamera.GetPosition();
    glm::vec3 camForward = m_RenderCamera.GetForward();

    Renderer::BeginScene(m_RenderCamera);

    // Cube click test
    static bool lastClick = false;

    bool click = Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || ControllerInput::IsButtonPressed(0, GamepadButton::R1);
    for (int i = 0; i < 16; i++) {
        if (ControllerInput::IsButtonPressed(0, (GamepadButton)i)) {
            WK_CORE_INFO("BUTTON PRESSED: {0}", i);
        }
    }

    if (click && !lastClick) {
        Ray ray = CreateCameraRay(m_RenderCamera, camPos);
        for (auto entity : playerView) {
            auto& playerTransform = playerView.get<Transform>(entity);
            ray.Direction = playerTransform.LocalOrientation * glm::vec3(0, 0, -1);
            ray.Origin = playerTransform.LocalPosition + glm::normalize(ray.Direction) * 0.7f;
        }

        RaycastHit hit;
        float maxDist = 1000.0f;
        bool hitBlock = false;

        if (RaycastAABB(m_Scene, ray, hit, maxDist)) {
            Entity e = hit.HitEntity;

            auto& registry = m_Scene.Registry();

            if (registry.all_of<Tag>(e.GetHandle())) {
                auto& tag = registry.get<Tag>(e.GetHandle());

                WK_CORE_INFO("Ray hit entity: {0}", tag.Name);

                // ONLY teleport cubes
                if (tag.Name == "Cube") {
                    hitBlock = true;
                    float range = 10.0f;
                    glm::vec3 newPos(RandomFloat() * range, 20.f + RandomFloat() * range, RandomFloat() * range);
                    registry.get<Transform>(e.GetHandle()).LocalPosition = newPos;
                }
            }
        }

        AudioSystem::Play(hitBlock ? m_ClickHitBeep : m_ClickMissBeep);
    }

    lastClick = click;


    auto view = m_Scene.Registry().view<Transform, MeshRenderer>();

    // X Y Z loop for tiling world
    for (int ix = -m_RepeatN; ix <= m_RepeatN; ix++) {
    for (int iy = -m_RepeatN; iy <= m_RepeatN; iy++) {
    for (int iz = -m_RepeatN; iz <= m_RepeatN; iz++) {
        glm::vec3 worldOffset = glm::vec3(ix, iy, iz) * m_ChunkSize;

        if (!(ix == 0 && iy == 0 && iz == 0)) {
            // check if chunk behind camera
            glm::vec3 chunkCenter = worldOffset;
            glm::vec3 toChunk = chunkCenter - camPos;
            float chunklen = glm::length(toChunk);
            if (chunklen > 0.001f) {
                float d = glm::dot(toChunk / chunklen, camForward);

                // Skip chunks behind camera
                if (d < -0.05f)
                    continue;
            }
        }

        // Render
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& mesh = view.get<MeshRenderer>(entity);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset) * transform.FinalTransform * mesh.GetLocalTransform();

            const Material* material = m_Scene.Registry().try_get<Material>(entity);
            Renderer::Submit(model, *mesh.MeshPtr, m_Shader.get(), material ? *material : Material{});
        }

        ColliderDebugDraw::Draw(m_Scene, worldOffset);

    } // Z
    } // Y
    } // X

    Renderer::EndScene();

    // HUD text - re-measured every frame so both labels stay anchored to
    // their corners across window resizes.
    if (m_TitleFont) {
        auto& window = Application::Get().GetWindow();
        uint32_t screenWidth = window.GetWidth();
        uint32_t screenHeight = window.GetHeight();
        const float padding = 20.0f;

        // Title, top-center.
        const std::string title = "Wankel";
        float titleWidth = m_TitleFont->MeasureWidth(title);
        glm::vec2 titlePos = {((float)screenWidth - titleWidth) * 0.5f, padding + 24.0f};
        Renderer::SubmitText(title, m_TitleFont, titlePos, screenWidth, screenHeight, {1.0f, 1.0f, 1.0f});

        // Mode indicator, bottom-right - reflects the player's current
        // look-mode directly from the component, so it switches on its own
        // whenever PlayerInputSystem toggles PlayerController::Mode.
        for (auto entity : playerView) {
            auto& controller = playerView.get<PlayerController>(entity);

            const std::string modeText = std::string("Mode: ") + (controller.Mode == PlayerController::LookMode::FPS ? "FPS" : "FLIGHT");
            float modeWidth = m_TitleFont->MeasureWidth(modeText);
            glm::vec2 modePos = {(float)screenWidth - padding - modeWidth, (float)screenHeight - padding};

            Renderer::SubmitText(modeText, m_TitleFont, modePos, screenWidth, screenHeight, {0.85f, 0.85f, 0.85f});
            break; // single local player
        }
    }
}


void SandboxLayer::OnImGuiRender() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->GetGlyphRangesDefault();
    //if (!m_GameFocused || m_GameFocused) {// making this always on now. Better tesing
    if (!m_GameFocused) {
        ImGui::Begin("Renderer Debug");
        DebugOverlay::DrawFPSPanel();

        // FOG
        if (ImGui::CollapsingHeader("Fog")) {
            ImGui::ColorEdit3("Fog Color", &m_Fog.Color[0], ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("Fog Density", &m_Fog.Density, 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

            ImGui::Separator();
            ImGui::Checkbox("Noise Fog", &m_Fog.NoiseEnabled);
            ImGui::SliderFloat("Noise Scale", &m_Fog.NoiseScale, 0.0001f, 0.1f, "%.5f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Noise Strength", &m_Fog.NoiseStrength, 0.0f, 4.0f);
            ImGui::SliderInt("Noise Octaves", &m_Fog.NoiseOctaves, 1, 8);

            ImGui::Separator();
            ImGui::Text("Fog Wind");
            ImGui::DragFloat3("Wind Direction", &m_Fog.WindDir[0], 0.01f);

            ImGui::SliderFloat("Wind Speed", &m_Fog.WindSpeed, 0.0f, 5.0f);

            // normalize direction to avoid weird scaling
            if (glm::length(m_Fog.WindDir) > 0.0001f)
                m_Fog.WindDir = glm::normalize(m_Fog.WindDir);
        }

        // LIGHTING
        if (ImGui::CollapsingHeader("Lighting")) {
            ImGui::DragFloat3("Light Direction", &m_Light.Direction[0], 0.01f);

            if (glm::length(m_Light.Direction) > 0.0001f)
                m_Light.Direction = glm::normalize(m_Light.Direction);

            ImGui::ColorEdit3("Light Color", &m_Light.Color[0], ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("Ambient", &m_Light.Ambient, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular", &m_Light.Specular, 0.0f, 1.0f);
        }

        // CAMERA (GLOBAL)
        if (ImGui::CollapsingHeader("Camera")) {
            auto camView = m_Scene.Registry().view<Transform, CameraComponent>();
            for (auto entity : camView) {
                auto& transform = camView.get<Transform>(entity);
                auto& camera = camView.get<CameraComponent>(entity);

                if (!camera.Primary)
                    continue;

                ImGui::Text("Camera Settings");
                ImGui::SliderFloat("FOV (Vertical)", &camera.FOV, 30.0f, 120.0f);
                ImGui::SliderFloat("Near Clip", &camera.Near, 0.001f, 5.0f);
                ImGui::SliderFloat("Far Clip", &camera.Far, 10.0f, 10000.0f);

                ImGui::Separator();
                glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(transform.LocalOrientation));
                ImGui::Text("Camera Transform");
                ImGui::DragFloat3("Offset", &transform.LocalPosition.x, 0.01f);
                if (ImGui::DragFloat3("Rotation Offset [pyr]", &eulerDeg.x, 0.1f)) {
                    transform.LocalOrientation = glm::quat(glm::radians(eulerDeg));
                }
            }
        }

        if (ImGui::CollapsingHeader("Entity")) {
            AnimationDebugPanel::Draw(m_Scene, m_SelectedAnimEntity);
        }


        // WORLD DEBUG
        if (ImGui::CollapsingHeader("World Tiling")) {
            ImGui::SliderFloat("Chunk Size", &m_ChunkSize, 1.0f, 500.0f, "%.1f");
            ImGui::SliderInt("Repeat N", &m_RepeatN, 0, 10);
            ImGui::Text("Grid: %d x %d x %d", 2 * m_RepeatN + 1, 2 * m_RepeatN + 1, 2 * m_RepeatN + 1);
        }

        ImGui::Checkbox("Debug Draw", &Renderer::DebugEnabled);


        // ENGINE INFO
        ImGui::Separator();
        ImGui::Text("Press ESC to return to game");
        ImGui::End();
    }
}


void SandboxLayer::OnEvent(Event& e) { // TODO: I DONT KNOW IF THIS SHOULD BE HERE, MAYBE ENGINE SHOULD HANDLE?
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
        //m_Controller.OnResize((float)e.GetWidth(), (float)e.GetHeight());
        m_RenderCamera.SetAspect((float)e.GetWidth() / (float)e.GetHeight());
        return false;
    });
}


} // namespace Wankel
