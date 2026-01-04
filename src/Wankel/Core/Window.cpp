#include "wkpch.h"
#include "Window.h"

#ifdef WK_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h" // does not exist yet
#endif
#ifdef WK_PLATFORM_LINUX
	#include "Platform/Linux/LinuxWindow.h" // does not exist yet
#endif

namespace Wankel { 
	Scope<Window> Window::Create(const WindowProps& props) {
	#ifdef WK_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
	#else
		return CreateScope<LinuxWindow>(props);
	#endif
	}
}
