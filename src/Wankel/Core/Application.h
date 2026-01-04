#pragma once

#include "Engine.h"
#include "Window.h"

namespace Wankel {
	class Application {

	public:
		Application();
		virtual ~Application();
	
		void Run();
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	// To be defined in client
	Application* CreateApplication();
}
