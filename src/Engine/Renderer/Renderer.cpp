
#include "Renderer.h"

#include <iostream>

// Binds the Shader, Vertex Array, and the Index Buffer, then issues a Draw Call
void Renderer::Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const {
    // Binds the Shader, Vertex Array (includes Vertex Buffer), and the Index Buffer
    shader.Bind();
    va.Bind();
    ib.Bind();

    // Issue Draw Call
    glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);
}

// Clears the screen
void Renderer::Clear() const {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::EnableBlending() {
    glEnable(GL_BLEND);
}

void Renderer::BlendFunction(GLenum sFactor, GLenum dFactor) {
    glBlendFunc(sFactor, dFactor);
}
