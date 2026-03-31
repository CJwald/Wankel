#pragma once

#include "Wankel/Core/Window.h"
//#include "Wankel/Renderer/GraphicsContext.h"
#include <glad/gl.h>     // Make sure this is included BEFORE GLFW
#include <GLFW/glfw3.h>

namespace Wankel {

	class LinuxWindow : public Window {
	public:
		LinuxWindow(const WindowProps& props);
		virtual ~LinuxWindow();

		void OnUpdate() override;

		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		//virtual void* GetNativeWindow() const { return m_Window; }
		//static Window* Create(const WindowProps& props = WindowProps());
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		//Scope<GraphicsContext> m_Context;

		struct WindowData {
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}
