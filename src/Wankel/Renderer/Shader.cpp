#include "Shader.h"
#include <glad/gl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

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
	        std::cout << "Shader compile error:\n" << info << std::endl;
	    }
	
	    return id;
	}
	
	Shader::Shader(const std::string& vertexSrcFile, const std::string& fragmentSrcFile) {
	    unsigned int program = glCreateProgram();
	
		std::string& vertexSrc = ReadFile(vertexSrcFile);
		std::string& fragmentSrc = ReadFile(fragmentSrcFile);

	    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
	    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
	
	    glAttachShader(program, vs);
	    glAttachShader(program, fs);
	    glLinkProgram(program);
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

	void Shader::SetMat4(const std::string& name, const glm::mat4& matrix) {
	    int loc = glGetUniformLocation(m_RendererID, name.c_str());
	    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
	}

}
