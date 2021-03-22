
#pragma once

#include <string>
#include <unordered_map>

struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};

class Shader {
private:
    unsigned int m_RendererID;
    std::string m_Filepath;
    std::unordered_map<std::string, unsigned int> m_UniformLocationCache;

    ShaderProgramSource ParseShader(const std::string &filepath);
    unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader);
    unsigned int CompileShader(unsigned int type, const std::string &source);

    unsigned int GetUniformLocation(const std::string &name);

public:
    explicit Shader(const std::string &filepath);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Uniform Manipulations
    void SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3);

};


