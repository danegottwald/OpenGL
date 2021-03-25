
#include "Renderer.h"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Clears the screen
void Renderer::Clear() const {
    glClear(GL_COLOR_BUFFER_BIT);
}

// Binds the Shader, Vertex Array, and the Index Buffer, then issues a Draw Call
void Renderer::Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const {
    // Binds the Shader, Vertex Array (includes Vertex Buffer), and the Index Buffer
    shader.Bind();
    va.Bind();
    ib.Bind();

    // Issue Draw Call
    glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::Present() {
    
}

void Renderer::EnableBlending(GLenum sFactor, GLenum dFactor) {
    glEnable(GL_BLEND);
    glBlendFunc(sFactor, dFactor);
}

