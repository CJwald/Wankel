#include <Wankel/Core/Application.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <iostream>

namespace Wankel {

Application* Application::s_Instance = nullptr;

Application::Application(const std::string& title, int width, int height)
{
    s_Instance = this;
    m_Width = width;
    m_Height = height;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_Window = SDL_CreateWindow(
        title.c_str(),
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!m_Window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return;
    }

    m_GLContext = SDL_GL_CreateContext(m_Window);
    if (!m_GLContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_GL_MakeCurrent(m_Window, m_GLContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // GLAD defines these macros
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f);

    m_LastTime = SDL_GetTicks();
    OnInit();
}

Application::~Application()
{
    OnShutdown();
    if (m_GLContext) SDL_GL_DestroyContext(m_GLContext);  // ← SDL3 correct name
    if (m_Window)    SDL_DestroyWindow(m_Window);
    SDL_Quit();
}

void Application::HandleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            m_Running = false;

        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            m_Width  = event.window.data1;
            m_Height = event.window.data2;
            glViewport(0, 0, m_Width, m_Height);
        }

        if (event.type == SDL_EVENT_WINDOW_MINIMIZED)  m_Minimized = true;
        if (event.type == SDL_EVENT_WINDOW_RESTORED)   m_Minimized = false;
    }
}

void Application::Run()
{
    while (m_Running)
    {
        uint64_t now = SDL_GetTicks();
        m_DeltaTime = (now - m_LastTime) / 1000.0f;
        m_LastTime = now;

        HandleEvents();

        if (!m_Minimized)
        {
            OnUpdate(m_DeltaTime);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            OnRender();
            OnImGui();

            SDL_GL_SwapWindow(m_Window);
        }
    }
}

} // namespace Wankel
