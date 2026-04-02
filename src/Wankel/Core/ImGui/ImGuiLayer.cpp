#include "wkpch.h"
#include "ImGuiLayer.h"

#include "Wankel/Core/Application.h"
#include "Wankel/Core/Window.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace Wankel {

	ImGuiLayer::ImGuiLayer()
	    : Layer("ImGuiLayer")
	{
	}
	
	void ImGuiLayer::OnAttach()
	{
	    IMGUI_CHECKVERSION();
	    ImGui::CreateContext();
	
	    ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		// Docking is now usually always enabled internally,
		// but keep this if defined:
		#ifdef ImGuiConfigFlags_DockingEnable
		    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		#endif
		
		#ifdef ImGuiConfigFlags_ViewportsEnable
		    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		#endif

	    ImGui::StyleColorsDark();
	
	    Application& app = Application::Get();
	    GLFWwindow* window =
	        static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
	
	    ImGui_ImplGlfw_InitForOpenGL(window, true);
	    ImGui_ImplOpenGL3_Init("#version 330");
	}
	
	void ImGuiLayer::OnDetach()
	{
	    ImGui_ImplOpenGL3_Shutdown();
	    ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
	}
	
	void ImGuiLayer::OnEvent(Event& e)
	{
	    ImGuiIO& io = ImGui::GetIO();
	
	    if (io.WantCaptureMouse && e.IsInCategory(EventCategoryMouse))
	        e.SetHandled(true);
	
	    if (io.WantCaptureKeyboard && e.IsInCategory(EventCategoryKeyboard))
	        e.SetHandled(true);
	}
	
	void ImGuiLayer::Begin()
	{
	    ImGui_ImplOpenGL3_NewFrame();
	    ImGui_ImplGlfw_NewFrame();
	    ImGui::NewFrame();
	}
	
	void ImGuiLayer::End()
	{
	    ImGuiIO& io = ImGui::GetIO();
	    Application& app = Application::Get();
	
	    io.DisplaySize = ImVec2(
	        (float)app.GetWindow().GetWidth(),
	        (float)app.GetWindow().GetHeight()
	    );
	
	    ImGui::Render();
	    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		#ifdef ImGuiConfigFlags_ViewportsEnable
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
		    GLFWwindow* backup = glfwGetCurrentContext();
		
		    #ifdef IMGUI_HAS_VIEWPORT
		    ImGui::UpdatePlatformWindows();
		    ImGui::RenderPlatformWindowsDefault();
		    #endif
		
		    glfwMakeContextCurrent(backup);
		}
		#endif

	}

}
