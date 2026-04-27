#include "SandboxLayer.h"

#include <Wankel.h>
#include <Wankel/Core/EntryPoint.h> // MUST INCLUDE TO DEFINE ENTRY POINT

class SandboxApp : public Wankel::Application {
public:
	SandboxApp() {
		PushLayer(new Wankel::SandboxLayer());
	}
};

Wankel::Application* Wankel::CreateApplication() {
	return new SandboxApp();
}

