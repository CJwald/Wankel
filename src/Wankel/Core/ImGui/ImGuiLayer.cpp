#include "wkpch.h"
#include "ImGuiLayer.h"

#include "Wankel/Core/Application.h"
#include "Wankel/Core/ImGui/ImGuiTheme.h"
#include "Wankel/Core/Window.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace Wankel {

//
ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

//
void ImGuiLayer::OnAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ApplyImGuiTheme();

    // Default font first (merge target), then the icon glyphs Icons.h defines, merged in from
    // NotoEmoji (OFL-licensed monochrome emoji font - see fonts/OFL.txt) at a size matching the
    // default font. Only this narrow range is loaded, not the font's full repertoire, to keep the
    // atlas small - see docs/IMGUIRefactor.md Phase 8.
    io.Fonts->AddFontDefault();

    static const ImWchar kIconGlyphRanges[] = {
        0x23ED,  0x23ED,  // Step
        0x23F8,  0x23F8,  // Pause
        0x25B6,  0x25B6,  // Play
        0x1F4A1, 0x1F4A1, // Light bulb
        0,
    };
    ImFontConfig iconFontConfig;
    iconFontConfig.MergeMode = true;
    iconFontConfig.PixelSnapH = true;
    // 0.0f = implicit reference size, so icons scale with the default font instead of a fixed size -
    // ImGui asserts if a MergeMode font requests an explicit size against an implicitly-sized target
    // (AddFontDefault() above), see imgui_draw.cpp's AddFont() sizing compatibility table.
    io.Fonts->AddFontFromFileTTF("WankelFonts/NotoEmoji.ttf", 0.0f, &iconFontConfig, kIconGlyphRanges);

    Application& app = Application::Get();
    GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

//
void ImGuiLayer::OnDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

//
void ImGuiLayer::OnEvent(Event& e) {
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse && e.IsInCategory(EventCategoryMouse))
        e.SetHandled(true);

    if (io.WantCaptureKeyboard && e.IsInCategory(EventCategoryKeyboard))
        e.SetHandled(true);
}

//
void ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

//
void ImGuiLayer::End() {
    ImGuiIO& io = ImGui::GetIO();
    Application& app = Application::Get();

    io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

    ImGui::Render();
    m_GpuTimer.Begin();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    m_GpuTimer.End();
    m_LastGpuMs = m_GpuTimer.PollElapsedMs();
}

} // namespace Wankel
