#pragma once

#include "Engine.h"
#include "Window.h"
#include <Wankel/Core/LayerStack.h>
#include <Wankel/Core/Events/Event.h>
#include <Wankel/Core/Events/ApplicationEvent.h>

namespace Wankel {
	class Application {

	public:
		Application();
		virtual ~Application();
	
		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;
	};

	// To be defined in client
	Application* CreateApplication();
}
