#include "wkpch.h"
#include "Wankel/Core/Platform/Linux/LinuxWindow.h"
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

	LinuxWindow::LinuxWindow(const WindowProps& props) {
		Init(props);
	}

	LinuxWindow::~LinuxWindow() {
		Shutdown();
	}

	void LinuxWindow::Init(const WindowProps& props) {

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.VSync = true;

		WK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0) {
			int success = glfwInit();
			WK_CORE_ASSERT(success, "Could not initialize GLFW!");
			s_GLFWWindowCount = 1;
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
		if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
		    WK_CORE_ERROR("Failed to initialize GLAD");
		    throw std::runtime_error("GLFW initialization failed");
		}
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
		m_Data.Width = fbWidth;
		m_Data.Height = fbHeight;

		glViewport(0, 0, fbWidth, fbHeight);

		//glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

		if (glfwRawMouseMotionSupported())
		    glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
		    if (!data) return;
		
			if (data->FirstMouse) {
			    data->LastMouseX = xPos;
			    data->LastMouseY = yPos;
			    data->FirstMouse = false;
			    return;
			}
			
			float prevX = data->LastMouseX;
			float prevY = data->LastMouseY;
			float deltaX = static_cast<float>(xPos - data->LastMouseX) * 0.002f;
			float deltaY = static_cast<float>(yPos - data->LastMouseY) * 0.002f;
			
			data->LastMouseX = xPos;
			data->LastMouseY = yPos;
		
			WK_CLIENT_TRACE(
			    "x={0:.3f}, y={1:.3f}, prevX={2:.3f}, prevY={3:.3f}, dX={4:.3f}, dY={5:.3f}",
			    xPos, yPos, prevX, prevY, deltaX, deltaY
			);

		    // Update Input system
		    Wankel::Input::SetMouseDelta(deltaX, -deltaY);   // negative Y for natural look
		
		    // Optional: fire event
		    MouseMovedEvent event(deltaX, deltaY);
		    data->EventCallback(event);
		});
	}

	//
	void LinuxWindow::Shutdown() {
		glfwDestroyWindow(m_Window);
		if (--s_GLFWWindowCount == 0)
    		glfwTerminate();

		if (s_GLFWWindowCount == 0) {
			glfwTerminate();
		}
	}

	//
	void LinuxWindow::OnUpdate() {
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

	//
	void LinuxWindow::SetVSync(bool enabled) {
		m_Data.VSync = enabled;
		glfwSwapInterval(enabled ? 1 : 0);
	}

	//
	bool LinuxWindow::IsVSync() const {
		return m_Data.VSync;
	}

}
