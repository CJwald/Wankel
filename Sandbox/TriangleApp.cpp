#include <Wankel.h>
#include <imgui.h>
#include <glad/gl.h>

#include "Wankel/Renderer/Shader.h"
#include "Wankel/Renderer/Buffer.h"
#include "Wankel/Renderer/VertexArray.h"

class TriangleLayer : public Wankel::Layer {
public:
    TriangleLayer()
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };

        m_VAO = new Wankel::VertexArray();
        m_VAO->Bind();

        m_VBO = new Wankel::VertexBuffer(vertices, sizeof(vertices));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(0);

        std::string vs = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )";

        std::string fs = R"(
            #version 330 core
            out vec4 color;
            void main() { color = vec4(0.2, 0.7, 0.3, 1.0); }
        )";

        m_Shader = new Wankel::Shader(vs, fs);
    }

    void OnUpdate() override
    {
        glClear(GL_COLOR_BUFFER_BIT);

        m_Shader->Bind();
        m_VAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    Wankel::VertexArray* m_VAO;
    Wankel::VertexBuffer* m_VBO;
    Wankel::Shader* m_Shader;
};

class TriangleApp : public Wankel::Application {
public:
    TriangleApp() { PushLayer(new TriangleLayer()); }
};

Wankel::Application* Wankel::CreateApplication()
{
    return new TriangleApp();
}
