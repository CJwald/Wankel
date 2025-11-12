#include <Wankel/Core/Engine.h>
#include <Wankel/Core/Application.h>

#ifdef WANKEL_CLIENT
#endif

class Sandbox : public Wankel::Application {
public:
    void OnInit() override {
        #ifdef WANKEL_CLIENT
        #endif
    }

    void OnUpdate(float dt) override {
        // Test logic
    }

    void OnShutdown() override {
        #ifdef WANKEL_CLIENT
        #endif
    }
};

WANKEL_MAIN(Sandbox)
