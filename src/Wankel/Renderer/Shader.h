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

    // False if compilation or linking failed (see the WK_CORE_ERROR logged at construction) - the
    // program is still a valid (if useless) GL object, so this doesn't throw/abort, it's a caller
    // opt-in check for cases that need to fail closed instead of silently drawing garbage, e.g. a
    // conditionally-compiled shader (see cube_gpu_split.frag) that must not be used if it didn't
    // actually build despite extension detection having said it should.
    bool IsValid() const { return m_LinkSucceeded; }

    void Bind() const;
    void Unbind() const;
    void SetMat4(const std::string& name, const glm::mat4& matrix);
    void SetMat3(const std::string& name, const glm::mat3& matrix);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);

private:
    int GetUniformLocation(const std::string& name);

private:
    unsigned int m_RendererID;
    bool m_LinkSucceeded = false;
    std::unordered_map<std::string, int> m_UniformLocationCache;
};

} // namespace Wankel
