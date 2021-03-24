
#include "Shader.h"

#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

// Create a attach the Shaders within the specified file
Shader::Shader(const std::string &filepath) : m_Filepath(filepath) {
    // Parse the Shader file into separate vertex and fragment shaders
    m_Source = ParseShader(filepath);
    // Returns an ID to reference the current Shader program
    m_RendererID = CreateShader(m_Source.VertexSource, m_Source.FragmentSource);
}

// Frees memory and restores the name taken by m_RendererID
Shader::~Shader() {
    glDeleteProgram(m_RendererID);
}

// Sets the Shader program as part of the current rendering state
void Shader::Bind() const {
    glUseProgram(m_RendererID);
}

// Removes the Shader program from part of the rendering state
void Shader::Unbind() const {
    glUseProgram(0);
}

// Specifies the value of a uniform variable in the current Shader program
void Shader::SetUniform1i(const std::string &name, int value) {
    glUniform1i(GetUniformLocation(name), value);
}

// Specifies the value of a uniform variable in the current Shader program
void Shader::SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3) {
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::SetUniformMat4f(const std::string &name, const glm::mat4 &matrix) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
}

// Returns the location of a uniform value within the Shader
int Shader::GetUniformLocation(const std::string &name) {
    // Search the cache to check if the uniform variable has been hit before
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) {
        return m_UniformLocationCache[name];
    }
    // Find the location of the uniform variable
    int location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1) {
        std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    }
    // Cache variable for future references
    m_UniformLocationCache[name] = location;
    return location;
}

// Parses the specified file into separate Shader source code
// Returns a struct that contains the source codes
Shader::ShaderProgramSource Shader::ParseShader(const std::string &filepath) {
    // Opens the file at 'filepath'
    std::ifstream stream(filepath);

    enum class ShaderType {
        NONE = -1,
        VERTEX = 0,
        FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line)) {
        if (line.find("#type") != std::string::npos) {
            if (line.find("vertex") != std::string::npos) {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos) {
                type = ShaderType::FRAGMENT;
            }
        }
        else if (type != ShaderType::NONE) {
            ss[(int) type] << line << '\n';
        }
    }
    return {
            ss[0].str(),
            ss[1].str()
    };
}

// Creates a Shader program, attaches a vertex and fragment shader, then links
// and validates the shader.
// Returns the Shader program's id
unsigned int Shader::CreateShader(const std::string &vertexShader, const std::string &fragmentShader) {
    // Creates an empty program object for which shader objects can be attached
    unsigned int program = glCreateProgram();

    // Creates shader objects for the Vertex and Fragment shaders
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    // Attaches the shader objects to the program object
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    // Links the program object and creates executables for each of the shaders
    glLinkProgram(program);
    // Checks/validates whether the executables contained in 'program' can execute
    glValidateProgram(program);

    // Flags the shaders for deletion, but will not be deleted until it is no longer
    // attached to any program object.  This will happen when glDeleteProgram() is
    // called on 'program'
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Returns the id for which the program object can be referenced
    return program;
}

// Compiles the Shader's source code
// Returns a Shader's id for attaching to a Shader program
unsigned int Shader::CompileShader(unsigned int type, const std::string &source) {
    // Create an empty shader object for the shader stage give by 'type'
    unsigned int id = glCreateShader(type);

    const char *src = source.c_str();
    // Set the shader's (id) source code
    glShaderSource(id, 1, &src, nullptr);
    // Compile the shader source code into the shader object
    glCompileShader(id);

    // Error Handling
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char *message = (char *) alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile "
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

