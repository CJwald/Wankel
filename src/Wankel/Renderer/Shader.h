#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Wankel {

	class Shader {
	public:
	    Shader(const std::string& vertexSrcFile, const std::string& fragmentSrcFile);
	    ~Shader();

	    Shader(const Shader&) = delete;
	    Shader& operator=(const Shader&) = delete;

	    void Bind() const;
	    void Unbind() const;
		void SetMat4(const std::string& name, const glm::mat4& matrix);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetFloat(const std::string& name, float value);
		void SetInt(const std::string& name, int value);

	private:
		int GetUniformLocation(const std::string& name);

	private:
	    unsigned int m_RendererID;
		std::unordered_map<std::string, int> m_UniformLocationCache;
	};

}
