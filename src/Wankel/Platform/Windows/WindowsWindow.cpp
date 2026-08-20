#include "wkpch.h"
#include "Wankel/Platform/Windows/WindowsWindow.h"
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Wankel/Core/Events/ApplicationEvent.h"
#include "Wankel/Core/Events/MouseEvent.h"
#include "Wankel/Core/Events/KeyEvent.h"


#include "Wankel/Core/Input.h"

namespace Wankel {


static uint8_t s_GLFWWindowCount = 0;


static void GLFWErrorCallback(int error, const char* description) {
    WK_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}


WindowsWindow::WindowsWindow(const WindowProps& props) {
    Init(props);
}


WindowsWindow::~WindowsWindow() {
    Shutdown();
}


void WindowsWindow::Init(const WindowProps& props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.VSync = false; //true;

    WK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

    if (s_GLFWWindowCount == 0) {
        int success = glfwInit();
        WK_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWWindowCount = 1;
    } else {
        ++s_GLFWWindowCount;
    }

    // 4.3 core - needed for SSBOs + glMultiDrawElementsIndirect (chunk draw-call batching).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        WK_CORE_ERROR("Failed to create GLFW window!");
        throw std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(m_Window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        WK_CORE_ERROR("Failed to initialize GLAD");
        throw std::runtime_error("GLFW initialization failed");
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    m_Data.Width = fbWidth;
    m_Data.Height = fbHeight;

    glViewport(0, 0, fbWidth, fbHeight);

    SetCursorMode(m_CursorMode);
    glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Print info so you know it worked
    WK_CORE_INFO("OpenGL Vendor:   {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    WK_CORE_INFO("OpenGL Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    WK_CORE_INFO("OpenGL Version:  {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(false);

    // Set GLFW callbacks
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        data.Width = width;
        data.Height = height;

        WindowResizeEvent event(width, height);
        data.EventCallback(event);
    });

    //
    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        WindowCloseEvent event;
        data.EventCallback(event);
    });

    //
    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event(button);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event(button);
                data.EventCallback(event);
                break;
            }
        }
    });

    //
    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        MouseScrolledEvent event((float)xOffset, (float)yOffset);
        data.EventCallback(event);
    });

    //
    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        if (!data)
            return;

        // Absolute position, converted to framebuffer-pixel space (same space Window::GetWidth/Height
        // and all screen-space rendering/UI-hit-testing use) - GLFW's cursor callback reports window
        // coordinates, which differ from framebuffer pixels whenever content scaling is active (any
        // HiDPI display), causing hit-testing to drift further off the further the cursor is from the
        // top-left corner. Always current regardless of the delta-tracking state below.
        int winWidth = 0, winHeight = 0, fbWidth = 0, fbHeight = 0;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float scaleX = winWidth > 0 ? (float)fbWidth / (float)winWidth : 1.0f;
        float scaleY = winHeight > 0 ? (float)fbHeight / (float)winHeight : 1.0f;
        Wankel::Input::SetMousePosition((float)xPos * scaleX, (float)yPos * scaleY);

        float dx, dy;

        if (data->FirstMouse) {
            data->LastMouseX = xPos;
            data->LastMouseY = yPos;
            data->FirstMouse = false;
            return;
        }

        dx = (float)(xPos - data->LastMouseX);
        dy = (float)(yPos - data->LastMouseY);
        data->LastMouseX = xPos;
        data->LastMouseY = yPos;

        Wankel::Input::SetMouseDelta(dx, dy);
    });
}


void WindowsWindow::Shutdown() {
    glfwDestroyWindow(m_Window);
    if (--s_GLFWWindowCount == 0)
        glfwTerminate();
}


void WindowsWindow::OnUpdate() {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);

    // Only update if changed (avoids redundant calls)
    if ((int)m_Data.Width != fbWidth || (int)m_Data.Height != fbHeight) {
        m_Data.Width = fbWidth;
        m_Data.Height = fbHeight;

        WindowResizeEvent event(fbWidth, fbHeight);
        m_Data.EventCallback(event);
    }

    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}


void WindowsWindow::SetVSync(bool enabled) {
    m_Data.VSync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}


bool WindowsWindow::IsVSync() const {
    return m_Data.VSync;
}


void WindowsWindow::SetFullscreen(bool enabled) {
    if (enabled == m_Fullscreen)
        return;

    if (enabled) {
        // glfwGetWindowPos is unsupported under native Wayland ("the platform does not provide the
        // window position" - by design, not a bug: Wayland never tells a client its absolute screen
        // position). Harmless to still call it - it's a no-op/logs a GLFW error there and the restored
        // position below is then just ignored by the compositor too, while X11/Windows get a real,
        // usable position back.
        glfwGetWindowPos(m_Window, &m_WindowedX, &m_WindowedY);
        glfwGetWindowSize(m_Window, &m_WindowedWidth, &m_WindowedHeight);

        // Real (monitor-argument) fullscreen, not a manual undecorate+resize "borderless" hack - tried
        // that first and it broke under Wayland (position-dependent calls above return garbage there,
        // producing a wrongly-scaled window). GLFW translates this into the compositor's native
        // xdg_toplevel fullscreen request under Wayland (already smooth/flicker-free there, no real
        // video-mode switch), and into an actual fullscreen video mode under X11/Windows.
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_Window, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight,
                             GLFW_DONT_CARE);
    }

    m_Fullscreen = enabled;
}


void WindowsWindow::SetCursorMode(CursorMode mode) {
    m_CursorMode = mode;

    int glfwMode = GLFW_CURSOR_DISABLED;
    switch (mode) {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Disabled:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
    }

    glfwSetInputMode(m_Window, GLFW_CURSOR, glfwMode);

    // Re-arm the cursor-pos callback's first-sample guard so re-entering
    // Disabled mode doesn't report a huge one-frame mouse delta from the
    // jump back to the window center.
    m_Data.FirstMouse = true;
}


} // namespace Wankel
