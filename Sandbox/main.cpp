#include <Wankel/Core/Engine.h>
#include <Wankel/Core/Application.h>

#ifdef WANKEL_CLIENT
#include <Wankel/Client/Rendering/Renderer.h>
#endif

class Sandbox : public Wankel::Application {
public:
    void OnInit() override {
        #ifdef WANKEL_CLIENT
        Wankel::Renderer::Init();
        #endif
    }

    void OnUpdate(float dt) override {
        // Test logic
    }

    void OnShutdown() override {
        #ifdef WANKEL_CLIENT
        Wankel::Renderer::Shutdown();
        #endif
    }
};

WANKEL_MAIN(Sandbox)
