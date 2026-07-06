#include "Shader.h"
#include <glad/gl.h>
#include <iostream>
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

		std::string vertexSrc = ReadFile(vertexSrcFile);
		std::string fragmentSrc = ReadFile(fragmentSrcFile);

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

	    glValidateProgram(program);

	    glDeleteShader(vs);
	    glDeleteShader(fs);

	    m_RendererID = program;
	}
	
	Shader::~Shader() {
	    glDeleteProgram(m_RendererID);
	}
	
	void Shader::Bind() const { glUseProgram(m_RendererID); }
	void Shader::Unbind() const { glUseProgram(0); }

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

}
