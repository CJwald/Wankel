#pragma once

#include "Wankel/Core/Window.h"
#include <GLFW/glfw3.h>

namespace Wankel {

    class WindowsWindow : public Window {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        unsigned int GetWidth() const override { return m_Data.Width; }
        unsigned int GetHeight() const override { return m_Data.Height; }

        void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        void* GetNativeWindow() const override { return m_Window; }

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

        GLFWwindow* m_Window;

        struct WindowData {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;

            float LastMouseX = 0.0f;
            float LastMouseY = 0.0f;
            bool FirstMouse = true;
        };

        WindowData m_Data;
    };

}
