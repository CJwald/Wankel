#pragma once

#include <exception>

extern Wankel::Application* Wankel::CreateApplication();

int main(int argc, char** argv) {
    Wankel::Log::Init(); // this wont live here forever, just a test
    WK_CORE_WARNING("Initialized Logger");
    WK_CLIENT_WARNING("Initialized Logger");
    WK_SERVER_WARNING("Initialized Logger");

    Wankel::Application* app = nullptr;

    // CreateApplication()/Run() can throw (e.g. a failed asset load or a
    // window/GL init failure) - catch it here so the app logs a clear
    // message and exits cleanly instead of an uncaught exception hitting
    // std::terminate() with no diagnostic and no window/context teardown.
    try {
        app = Wankel::CreateApplication();
        app->Run();
    } catch (const std::exception& e) {
        WK_CORE_FATAL("Unhandled exception, shutting down: {0}", e.what());
        delete app;
        return 1;
    } catch (...) {
        WK_CORE_FATAL("Unhandled unknown exception, shutting down");
        delete app;
        return 1;
    }

    delete app;
    return 0;
}
