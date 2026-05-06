#pragma once
#include <string>
#include <glm/glm.hpp>

namespace Wankel {

	class Shader {
	public:
	    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
	    ~Shader();
	
	    void Bind() const;
	    void Unbind() const;
		void SetMat4(const std::string& name, const glm::mat4& matrix);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetFloat(const std::string& name, float value);
	
	private:
	    unsigned int m_RendererID;
	};

}
