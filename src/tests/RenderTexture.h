#pragma once

#include "Test.h"

#include "../Game/Renderer/Renderer.h"
#include "../Game/Renderer/Texture.h"

#include "glm/glm.hpp"

namespace TestSpace
{

class RenderTexture : public Test
{
private:
    VertexBuffer* vb;
    VertexArray* va;
    VertexBufferLayout layout;
    IndexBuffer* ib;
    Shader* shader;
    Texture* texture;
    glm::vec3 translation;

public:
    RenderTexture();
    ~RenderTexture();

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;
};

}  // namespace TestSpace
