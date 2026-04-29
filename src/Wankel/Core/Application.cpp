#include "wkpch.h"
#include "Wankel/Core/Application.h"
#include "Wankel/Core/ImGui/ImGuiLayer.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Renderer/Renderer.h"


namespace Wankel {
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application::Application() {
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();
		
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application() {
	}

	void Application::PushLayer(Layer* layer) {
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}
	
	void Application::PushOverlay(Layer* layer) {
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Run() {
		while (m_Running) {

			Renderer::Clear(0.1f, 0.1f, 0.1f, 1.0f);

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();
			
			m_ImGuiLayer->Begin();

    		for (Layer* layer : m_LayerStack)
    		    layer->OnImGuiRender(); // optional but recommended

    		m_ImGuiLayer->End();

			m_Window->OnUpdate();
			Input::ResetMouseDelta(); // TODO: This may need to be removed
		}
	}
	
	void Application::OnEvent(Event& event) { 
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			(*--it)->OnEvent(event);
			if (event.IsHandled())
				break;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
	    Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
	    return false;
	}

	Application* Application::s_Instance = nullptr;

}



