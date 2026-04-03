#include "wkpch.h"
#include "Window.h"

#ifdef WK_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif
#ifdef WK_PLATFORM_LINUX
	#include "Platform/Linux/LinuxWindow.h"
#endif

namespace Wankel { 
	Scope<Window> Window::Create(const WindowProps& props) {
	#ifdef WK_PLATFORM_WINDOWS
	    return CreateScope<WindowsWindow>(props);
	#elif defined(WK_PLATFORM_LINUX)
	    return CreateScope<LinuxWindow>(props);
	#else
	    static_assert(false, "Unknown platform!");
	#endif
	}
}
