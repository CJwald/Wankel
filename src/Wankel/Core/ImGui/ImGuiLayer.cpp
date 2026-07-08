#include "wkpch.h"
#include "ImGuiLayer.h"

#include "Wankel/Core/Application.h"
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

    // Docking/multi-viewport require ImGui's docking branch; external/imgui
    // is vendored at plain master, which doesn't declare
    // ImGuiConfigFlags_DockingEnable/ViewportsEnable at all. Re-add these
    // once (if) the submodule is switched to the docking branch.

    ImGui::StyleColorsDark();

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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace Wankel
