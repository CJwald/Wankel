#pragma once

#include "Engine.h"
#include "Window.h"
#include <Wankel/Core/LayerStack.h>
#include <Wankel/Core/Events/Event.h>
#include <Wankel/Core/Events/ApplicationEvent.h>

namespace Wankel {
	class ImGuiLayer;
	
	class Application {

	public:
		Application();
		virtual ~Application();
	
		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;
		
		ImGuiLayer* m_ImGuiLayer = nullptr;
		static Application* s_Instance;
	};


	// To be defined in client
	Application* CreateApplication();

}
