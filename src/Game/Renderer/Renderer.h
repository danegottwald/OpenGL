#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

class Renderer
{
private:
    std::unique_ptr<Shader> m_Shader;

    GLuint m_VA, m_VB, m_IB;

public:
    Renderer();
    ~Renderer();

    // Member Functions
    void Attach();
    void Detach();
    void Draw();
    void DrawGui();

    void SwapShader( const std::string& file );

    void UpdateViewport( unsigned int x, unsigned int y, unsigned int width,
        unsigned int height );
};
