#include "Shader.h"
#include <glad/gl.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

#include <Wankel/Core/Log.h>

// For reading shaders
static std::string ReadFile(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    if (!in)
        throw std::runtime_error("Failed to open file: " + filepath);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Expands `#include "path"` lines, resolved against the including file's directory, then the working directory.
static std::string ReadShaderSource(const std::string& filepath, int depth = 0) {
    constexpr int kMaxIncludeDepth = 8;
    std::string source = ReadFile(filepath);
    if (depth >= kMaxIncludeDepth)
        throw std::runtime_error("Shader include depth exceeded at: " + filepath);

    std::istringstream lines(source);
    std::ostringstream out;
    std::string line;
    while (std::getline(lines, line)) {
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos || line.compare(firstChar, 8, "#include") != 0) {
            out << line << '\n';
            continue;
        }

        size_t open = line.find('"', firstChar);
        size_t close = open == std::string::npos ? std::string::npos : line.find('"', open + 1);
        if (close == std::string::npos)
            throw std::runtime_error(std::string("Malformed #include in ").append(filepath).append(": ").append(line));

        std::filesystem::path includeName = line.substr(open + 1, close - open - 1);
        std::filesystem::path relative = std::filesystem::path(filepath).parent_path() / includeName;
        out << ReadShaderSource(std::filesystem::exists(relative) ? relative.string() : includeName.string(), depth + 1)
            << '\n';
    }
    return out.str();
}

namespace Wankel {

static unsigned int CompileShader(unsigned int type, const std::string& src) {
    unsigned int id = glCreateShader(type);
    const char* source = src.c_str();
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (!result) {
        char info[512];
        glGetShaderInfoLog(id, 512, nullptr, info);
        WK_CORE_ERROR("Shader compile error:\n{0}", info);
    }

    return id;
}

Shader::Shader(const std::string& vertexSrcFile, const std::string& fragmentSrcFile) {
    unsigned int program = glCreateProgram();

    std::string vertexSrc = ReadShaderSource(vertexSrcFile);
    std::string fragmentSrc = ReadShaderSource(fragmentSrcFile);

    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int linkResult;
    glGetProgramiv(program, GL_LINK_STATUS, &linkResult);
    if (!linkResult) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        WK_CORE_ERROR("Shader link error ({0}, {1}):\n{2}", vertexSrcFile, fragmentSrcFile, info);
    }
    m_LinkSucceeded = linkResult != 0;

    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    m_RendererID = program;
}

namespace {
unsigned int s_BoundProgram = 0; // GL is global state - one cache shared by every Shader instance
}

Shader::~Shader() {
    if (s_BoundProgram == m_RendererID)
        s_BoundProgram = 0; // avoid a stale cache hit if a later program reuses this GL name
    glDeleteProgram(m_RendererID);
}

void Shader::Bind() const {
    if (s_BoundProgram == m_RendererID)
        return;
    glUseProgram(m_RendererID);
    s_BoundProgram = m_RendererID;
}
void Shader::Unbind() const {
    glUseProgram(0);
    s_BoundProgram = 0;
}

int Shader::GetUniformLocation(const std::string& name) {
    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end())
        return it->second;

    int loc = glGetUniformLocation(m_RendererID, name.c_str());
    m_UniformLocationCache[name] = loc;

    return loc;
}

void Shader::SetMat4(const std::string& name, const glm::mat4& matrix) {
    int loc = GetUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetMat3(const std::string& name, const glm::mat3& matrix) {
    int loc = GetUniformLocation(name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    int loc = GetUniformLocation(name);
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::SetFloat(const std::string& name, float value) {
    int loc = GetUniformLocation(name);
    glUniform1f(loc, value);
}

void Shader::SetInt(const std::string& name, int value) {
    int loc = GetUniformLocation(name);
    glUniform1i(loc, value);
}

} // namespace Wankel
