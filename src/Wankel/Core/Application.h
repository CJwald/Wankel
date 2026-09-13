#pragma once

#include "Engine.h"
#include "Window.h"
#include <Wankel/Core/Assert.h>
#include <Wankel/Core/LayerStack.h>
#include <Wankel/Core/Events/Event.h>
#include <Wankel/Core/Events/ApplicationEvent.h>

namespace Wankel {
class ImGuiLayer;

// Headless = true skips Window/Renderer/AudioSystem/gamepad-input construction entirely - for a
// dedicated server process with no display. TargetTickRate only applies in headless mode: with no
// Window::OnUpdate() vsync wait to pace the loop, Run() sleeps to hit this rate instead. A single
// config value rather than a constant so a server can move from a 60Hz dev target down to 30Hz for
// a real release without touching Run() itself.
struct ApplicationSpecification {
    std::string Name = "Wankel Application";
    bool Headless = false;
    double TargetTickRate = 60.0;
};

class Application {
public:
    explicit Application(const ApplicationSpecification& spec = {});
    virtual ~Application();

    void Run();

    void OnEvent(Event& e);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    static Application& Get() { return *s_Instance; }
    bool IsHeadless() const { return m_Spec.Headless; }
    // Asserts if called in headless mode, where there is no Window - see ApplicationSpecification.
    Window& GetWindow() { WK_CORE_ASSERT(m_Window, "Application::GetWindow: running headless"); return *m_Window; }
    ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    ApplicationSpecification m_Spec;
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    LayerStack m_LayerStack;

    ImGuiLayer* m_ImGuiLayer = nullptr;
    static Application* s_Instance;
};


// To be defined in client
Application* CreateApplication();

} // namespace Wankel
