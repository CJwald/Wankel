#pragma once

#include "Wankel/Core/Window.h"
#include <glad/gl.h> // Make sure this is included BEFORE GLFW
#include <GLFW/glfw3.h>

namespace Wankel {

class LinuxWindow : public Window {
public:
    LinuxWindow(const WindowProps& props);
    virtual ~LinuxWindow();

    void OnUpdate() override;

    unsigned int GetWidth() const override { return m_Data.Width; }
    unsigned int GetHeight() const override { return m_Data.Height; }

    // Window attributes
    void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
    void SetVSync(bool enabled) override;
    bool IsVSync() const override;
    void* GetNativeWindow() const override { return m_Window; }

    void SetCursorMode(CursorMode mode) override;
    CursorMode GetCursorMode() const override { return m_CursorMode; }

    void SetFullscreen(bool enabled) override;
    bool IsFullscreen() const override { return m_Fullscreen; }

private:
    virtual void Init(const WindowProps& props);
    virtual void Shutdown();

private:
    GLFWwindow* m_Window;
    CursorMode m_CursorMode = CursorMode::Disabled;

    // Windowed geometry to restore when leaving fullscreen - captured the moment SetFullscreen(true)
    // is called, since GLFW itself doesn't track "the size/pos before we last touched them".
    bool m_Fullscreen = false;
    int m_WindowedX = 0, m_WindowedY = 0;
    int m_WindowedWidth = 0, m_WindowedHeight = 0;

    double m_LastMouseX = 0.0; // TODO: Remove? Set in WindowData
    double m_LastMouseY = 0.0; // TODO: Remove? Set in WindowData
    bool m_FirstMouse = true;  // TODO: Remove? Set in WindowData

    struct WindowData {
        std::string Title;
        unsigned int Width, Height;
        bool VSync;

        EventCallbackFn EventCallback;

        double LastMouseX = 400.0; // reasonable starting point
        double LastMouseY = 300.0;
        bool FirstMouse = true;
    };

    WindowData m_Data;
};

} // namespace Wankel
