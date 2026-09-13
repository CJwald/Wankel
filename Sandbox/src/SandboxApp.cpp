#include "SandboxLayer.h"

#ifdef WANKEL_NET_DEMO
#include "NetDemo/NetDemoLayer.h"
#endif

#include <Wankel.h>
#include <Wankel/Core/EntryPoint.h> // MUST INCLUDE TO DEFINE ENTRY POINT

class SandboxApp : public Wankel::Application {
public:
    SandboxApp() {
        PushLayer(new Wankel::SandboxLayer());
#ifdef WANKEL_NET_DEMO
        // Verification-only: proves the Networking module (NetHost/NetMessageBus/ReplicationSystem)
        // end to end over an in-process ENet loopback. See Sandbox/src/NetDemo/NetDemoLayer.h.
        PushLayer(new Wankel::NetDemoLayer());
#endif
    }
};

Wankel::Application* Wankel::CreateApplication() {
    return new SandboxApp();
}
