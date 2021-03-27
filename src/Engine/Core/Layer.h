
#pragma once

#include <GL/glew.h>
#include <vector>

#include "glm/glm.hpp"
#include "../Engine/Renderer/Shader.h"

class Layer {
private:
    GLuint m_VAID;
    GLuint m_VBID;
    GLuint m_IBID;
    Shader* m_Shader;

    struct Vertex {
        glm::vec3 Position;
        glm::vec4 Color;
    };
    Vertex m_Vertex;

public:
    Layer();
    ~Layer();

    void Enable();
    void Draw();

};

