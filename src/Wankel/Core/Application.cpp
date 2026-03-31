#include <Wankel/Core/Application.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h> // todo: remove, just used for testing 

namespace Wankel {
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application::Application() {
		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
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
	
	void Application::OnEvent(Event& e) { 
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		WK_CORE_TRACE("{0}", e.ToString());	
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}
}



