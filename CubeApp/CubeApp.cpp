#include <Wankel.h>
#include <Wankel/Renderer/Shader.h>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>

using namespace Wankel;

// --------------------
// Helper: Read file
// --------------------
static std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// --------------------
// Cube Layer
// --------------------
class CubeLayer : public Layer {
public:
    CubeLayer()
        : Layer("CubeLayer")
    {
        float vertices[] = {
            -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
             0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,

            -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
             0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,

            -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
            -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

             0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
             0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,

            -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
             0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,

            -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
             0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
        };

        // VAO + VBO
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(0);

        // Shader
        std::string vert = ReadFile("../CubeApp/shaders/cube.vert");
        std::string frag = ReadFile("../CubeApp/shaders/cube.frag");
        m_Shader = new Shader(vert, frag);

        glEnable(GL_DEPTH_TEST);

        // --------------------
        // Camera setup
        // --------------------
        m_CameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
        m_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        m_CameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

        m_Yaw = -90.0f;
        m_Pitch = 0.0f;

        // --------------------
        // Mouse setup
        // --------------------
        GLFWwindow* window = static_cast<GLFWwindow*>(
            Application::Get().GetWindow().GetNativeWindow());

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetCursorPos(window, width / 2.0, height / 2.0);

        m_LastX = width / 2.0f;
        m_LastY = height / 2.0f;
        m_FirstMouse = true;
    }

    void OnUpdate() override
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(
            Application::Get().GetWindow().GetNativeWindow());

        // --------------------
        // Delta time
        // --------------------
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - m_LastFrame;
        m_LastFrame = currentTime;

        float speed = 2.5f * deltaTime;

        // --------------------
        // Movement
        // --------------------
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            m_CameraPos += speed * m_CameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            m_CameraPos -= speed * m_CameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            m_CameraPos -= glm::normalize(glm::cross(m_CameraFront, m_CameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            m_CameraPos += glm::normalize(glm::cross(m_CameraFront, m_CameraUp)) * speed;

        // --------------------
        // Mouse look
        // --------------------
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (m_FirstMouse)
        {
            m_LastX = xpos;
            m_LastY = ypos;
            m_FirstMouse = false;
        }

        float xoffset = xpos - m_LastX;
        float yoffset = m_LastY - ypos;

        m_LastX = xpos;
        m_LastY = ypos;

        float sensitivity = 0.002f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        m_Yaw   += xoffset;
        m_Pitch += yoffset;

        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        direction.y = sin(glm::radians(m_Pitch));
        direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_CameraFront = glm::normalize(direction);

        // --------------------
        // Rendering
        // --------------------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_Shader->Bind();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            1280.0f / 720.0f,
            0.1f,
            100.0f
        );

        glm::mat4 view = glm::lookAt(
            m_CameraPos,
            m_CameraPos + m_CameraFront,
            m_CameraUp
        );

        glm::mat4 model = glm::mat4(1.0f);

        m_Shader->SetMat4("projection", projection);
        m_Shader->SetMat4("view", view);
        m_Shader->SetMat4("model", model);

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

private:
    unsigned int m_VAO, m_VBO;
    Shader* m_Shader;

    glm::vec3 m_CameraPos;
    glm::vec3 m_CameraFront;
    glm::vec3 m_CameraUp;

    float m_Yaw;
    float m_Pitch;

    float m_LastX;
    float m_LastY;
    bool m_FirstMouse;

    float m_LastFrame = 0.0f;
};

// --------------------
// App
// --------------------
class CubeApp : public Application {
public:
    CubeApp() {
        PushLayer(new CubeLayer());
    }
};

Application* Wankel::CreateApplication() {
    return new CubeApp();
}
