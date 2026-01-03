#pragma once

#include "Engine.h"

namespace Wankel {
	class Application {

	public:
		Application();
		virtual ~Application();
	
		void Run();
	};

	// To be defined in client
	Application* CreateApplication();
}
