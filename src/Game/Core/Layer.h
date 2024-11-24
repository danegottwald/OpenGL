#pragma once

#include "../Renderer/Shader.h"
#include "../Renderer/Texture.h"

class Layer
{
public:
    Layer();
    ~Layer();

    void Enable();
    void Draw( float deltaTime );

    void LoadModel( const std::string& filename );

private:
    GLuint m_VAID, m_VBID, m_IBID;
    std::shared_ptr<Shader> m_Shader;
    std::shared_ptr<Texture> m_Texture;

};
