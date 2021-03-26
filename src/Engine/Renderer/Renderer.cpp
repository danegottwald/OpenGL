
#include "Renderer.h"

Renderer::Renderer() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

Renderer::~Renderer() {
}

// Clears the screen
void Renderer::Clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Binds the Shader, Vertex Array, and the Index Buffer, then issues a Draw Call
void Renderer::Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const {
    // Binds the Shader, Vertex Array (includes Vertex Buffer), and the Index Buffer
    shader.Bind();
    va.Bind();
    ib.Bind();
    
    // Issue Draw Call
    glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::UpdateViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    glViewport(x, y, width, height);
}


