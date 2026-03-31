#include "wkpch.h"
#include "Wankel/Core/Platform/Linux/LinuxWindow.h"
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//#include "Wankel/Core/Input.h"

#include "Wankel/Core/Events/ApplicationEvent.h"
#include "Wankel/Core/Events/MouseEvent.h"
#include "Wankel/Core/Events/KeyEvent.h"

//#include "Wankel/Renderer/Renderer.h"

//#include "Platform/OpenGL/OpenGLContext.h"

namespace Wankel {
	
	static uint8_t s_GLFWWindowCount = 0;

	Window* Window::Create(const WindowProps& props) {
		return new LinuxWindow(props);
	}
	//static void GLFWErrorCallback(int error, const char* description) {
	//	WK_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	//}

	LinuxWindow::LinuxWindow(const WindowProps& props) {
		//WK_PROFILE_FUNCTION();

		Init(props);
	}

	LinuxWindow::~LinuxWindow() {
		//WK_PROFILE_FUNCTION();

		Shutdown();
	}

	void LinuxWindow::Init(const WindowProps& props) {
		//WK_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.VSync = true;

		WK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0) {
			//WK_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			WK_CORE_ASSERT(success, "Could not initialize GLFW!");
			s_GLFWWindowCount = 1;
			//glfwSetErrorCallback(GLFWErrorCallback);
		} else {
			++s_GLFWWindowCount;
		}

		// Optional but recommended for modern OpenGL
    	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
		if (!m_Window) {
		    WK_CORE_ERROR("Failed to create GLFW window!");
		    throw std::runtime_error("GLFW window creation failed");
		}


		glfwMakeContextCurrent(m_Window);
		//m_Context = GraphicsContext::Create(m_Window);
		//m_Context->Init();

		// Load GLAD - this must come AFTER MakeContextCurrent
		if (!gladLoadGL(glfwGetProcAddress)) {
			WK_CORE_ERROR("Failed to initialize GLAD!");
		    return;
		}

		// Print info so you know it worked
		WK_CORE_INFO("OpenGL Vendor:   {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		WK_CORE_INFO("OpenGL Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		WK_CORE_INFO("OpenGL Version:  {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		//// Set GLFW callbacks
		//glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		//	data.Width = width;
		//	data.Height = height;

		//	WindowResizeEvent event(width, height);
		//	data.EventCallback(event);
		//});

		//glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		//	WindowCloseEvent event;
		//	data.EventCallback(event);
		//});

		//glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		//	switch (action) {
		//		case GLFW_PRESS: {
		//			KeyPressedEvent event(key, 0);
		//			data.EventCallback(event);
		//			break;
		//		}
		//		case GLFW_RELEASE: {
		//			KeyReleasedEvent event(key);
		//			data.EventCallback(event);
		//			break;
		//		}
		//		case GLFW_REPEAT: {
		//			KeyPressedEvent event(key, true);
		//			data.EventCallback(event);
		//			break;
		//		}
		//	}
		//});

		//glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		//	KeyTypedEvent event(keycode);
		//	data.EventCallback(event);
		//});

		//glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		//	switch (action) {
		//		case GLFW_PRESS: {
		//			MouseButtonPressedEvent event(button);
		//			data.EventCallback(event);
		//			break;
		//		}
		//		case GLFW_RELEASE: {
		//			MouseButtonReleasedEvent event(button);
		//			data.EventCallback(event);
		//			break;
		//		}
		//	}
		//});

		//glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		//	MouseScrolledEvent event((float)xOffset, (float)yOffset);
		//	data.EventCallback(event);
		//});

		//glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
		//	WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		//	MouseMovedEvent event((float)xPos, (float)yPos);
		//	data.EventCallback(event);
		//});
	}

	void LinuxWindow::Shutdown() {
		//WK_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		if (--s_GLFWWindowCount == 0)
    		glfwTerminate();

		if (s_GLFWWindowCount == 0) {
			glfwTerminate();
		}
	}

	void LinuxWindow::OnUpdate() {
		//WK_PROFILE_FUNCTION();

		glfwPollEvents();
		glfwSwapBuffers(m_Window);
		//m_Context->SwapBuffers();
	}

	void LinuxWindow::SetVSync(bool enabled) {
		//WK_PROFILE_FUNCTION();

		m_Data.VSync = enabled;
		glfwSwapInterval(enabled ? 1 : 0);
	}

	bool LinuxWindow::IsVSync() const {
		return m_Data.VSync;
	}

}
