#include <Wankel/Core/Application.h>

#include <SDL3/SDL.h>
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
			glClearColor(0, 1, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Window->OnUpdate();
		}
	}
}



