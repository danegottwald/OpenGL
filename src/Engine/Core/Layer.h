
#pragma once

#include <GL/glew.h>

#include "../Engine/Renderer/Shader.h"

class Layer {
private:
    GLuint m_VAID, m_VBID, m_IBID;
    Shader* m_Shader;

public:
    Layer();
    ~Layer();

    void Enable();
    void Draw(float frametime);

};

