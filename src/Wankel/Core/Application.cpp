#include <Wankel/Core/Application.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h> // todo: remove, just used for testing 

namespace Wankel {

	Application::Application() {
		m_Window = std::unique_ptr<Window>(Window::Create());
	}

	Application::~Application() {
	}
	
	void Application::Run() {
		while (m_Running) {
			std::cout << "Before glClearColor" << std::endl;
			if (glfwGetCurrentContext() == nullptr) {
			    std::cerr << "ERROR: No GLFW context is current!" << std::endl;
			    return;  // 🚨 STOP execution
			}	
			std::cout << "OpenGL Version: " << (const char*)glGetString(GL_VERSION) << std::endl;
			glClearColor(0, 1, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Window->OnUpdate();
			std::cout << "End app" << std::endl;
		}
	}
}



