#include "Shader.h"
#include <glad/gl.h>
#include <iostream>

namespace Wankel {

	static unsigned int CompileShader(unsigned int type, const std::string& src)
	{
	    unsigned int id = glCreateShader(type);
	    const char* source = src.c_str();
	    glShaderSource(id, 1, &source, nullptr);
	    glCompileShader(id);
	
	    int result;
	    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	    if (!result)
	    {
	        char info[512];
	        glGetShaderInfoLog(id, 512, nullptr, info);
	        std::cout << "Shader compile error:\n" << info << std::endl;
	    }
	
	    return id;
	}
	
	Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
	    unsigned int program = glCreateProgram();
	
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
	
	Shader::~Shader()
	{
	    glDeleteProgram(m_RendererID);
	}
	
	void Shader::Bind() const { glUseProgram(m_RendererID); }
	void Shader::Unbind() const { glUseProgram(0); }

}
