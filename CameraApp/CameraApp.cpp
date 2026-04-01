#include <Wankel.h>

#include "Wankel/Renderer/Shader.h"
#include "Wankel/Renderer/Buffer.h"
#include "Wankel/Renderer/VertexArray.h"
#include "Wankel/Renderer/Renderer.h"
#include "Wankel/Renderer/Camera.h"

class CameraLayer : public Wankel::Layer {
public:
    CameraLayer()
        : m_Camera(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };

        m_VAO = new Wankel::VertexArray();
        m_VAO->Bind();

        m_VBO = new Wankel::VertexBuffer(vertices, sizeof(vertices));
        m_VAO->AddLayout();

        std::string vs = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            uniform mat4 u_MVP;
            void main() {
                gl_Position = u_MVP * vec4(aPos, 1.0);
            }
        )";

        std::string fs = R"(
            #version 330 core
            out vec4 color;
            void main() {
                color = vec4(0.8, 0.3, 0.2, 1.0);
            }
        )";

        m_Shader = new Wankel::Shader(vs, fs);
    }

    void OnUpdate() override
    {
        Wankel::Renderer::Clear();

        m_Shader->Bind();

        glm::mat4 mvp = m_Camera.GetViewProjection();
        m_Shader->SetMat4("u_MVP", mvp);

        Wankel::Renderer::Draw(m_VAO, 3);
    }

private:
    Wankel::VertexArray* m_VAO;
    Wankel::VertexBuffer* m_VBO;
    Wankel::Shader* m_Shader;
    Wankel::Camera m_Camera;
};

class CameraApp : public Wankel::Application {
public:
    CameraApp() { PushLayer(new CameraLayer()); }
};

Wankel::Application* Wankel::CreateApplication()
{
    return new CameraApp();
}
