#include "HeadlessServerLayer.h"

#include <Wankel.h>
#include <Wankel/Core/EntryPoint.h> // MUST INCLUDE TO DEFINE ENTRY POINT

// Standalone process (separate executable target from Sandbox - see Sandbox/CMakeLists.txt's
// WANKEL_NET_DEMO option) proving ApplicationSpecification::Headless works end to end: no
// Window/Renderer/AudioSystem/gamepad input, Run() self-paced at TargetTickRate instead of vsync.
class HeadlessServerApp : public Wankel::Application {
public:
    HeadlessServerApp() : Wankel::Application({"HeadlessServerDemo", /*Headless=*/true, /*TargetTickRate=*/30.0}) {
        PushLayer(new Wankel::HeadlessServerLayer());
    }
};

Wankel::Application* Wankel::CreateApplication() {
    return new HeadlessServerApp();
}
