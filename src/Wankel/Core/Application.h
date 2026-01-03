#pragma once

#include <string>
#include <cstdint>

struct SDL_Window;

// SDL3: SDL_GLContext is an opaque typedef → we just forward-declare it
// DO NOT "using SDL_GLContext = void*" — it conflicts with SDL3's typedef
typedef struct SDL_GLContextState* SDL_GLContext;

namespace Wankel {

class Application
{
public:
    Application(const std::string& title = "Wankel Engine — @CJ_wald",
                int width = 1600, int height = 900);
    virtual ~Application();

    void Run();

    virtual void OnInit()    {}
    virtual void OnUpdate(float dt) {}
    virtual void OnRender()  {}
    virtual void OnImGui()   {}
    virtual void OnShutdown(){}

    SDL_Window* GetWindow() const { return m_Window; }
    int GetWidth()  const { return m_Width; }
    int GetHeight() const { return m_Height; }
    float GetDeltaTime() const { return m_DeltaTime; }

    static Application* Get() { return s_Instance; }

private:
    void HandleEvents();

    static Application* s_Instance;

    SDL_Window*     m_Window = nullptr;
    SDL_GLContext   m_GLContext = nullptr;

    bool     m_Running = true;
    bool     m_Minimized = false;

    int      m_Width = 1600;
    int      m_Height = 900;
    float    m_DeltaTime = 0.0f;
    uint64_t m_LastTime = 0;
};

} // namespace Wankel
