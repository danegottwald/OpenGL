
#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

class Shader {
private:
    // Holds the separate source codes for each Shader
    struct ShaderProgramSource {
        std::string VertexSource;
        std::string FragmentSource;
    };

    // Member Variables
    unsigned int m_RendererID;
    std::string m_File;
    ShaderProgramSource m_Source;
    std::unordered_map<std::string, unsigned int> m_UniformLocationCache;

    // Member Functions
    ShaderProgramSource ParseShader(const std::string &file);
    unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader);
    unsigned int CompileShader(unsigned int type, const std::string &source);

    int GetUniformLocation(const std::string &name);

public:
    // Constructor
    explicit Shader(const std::string &file);
    // Destructor
    ~Shader();

    // Member Functions
    void Bind() const;
    void Unbind() const;

    // Uniform Manipulations
    void SetUniform1i(const std::string &name, int value);
    void SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3);
    void SetUniformMat4f(const std::string &name, const glm::mat4 &matrix);

};


