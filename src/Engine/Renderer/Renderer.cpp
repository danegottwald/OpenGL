
#include "Renderer.h"

#include <iostream>

void GLAPIENTRY
ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
              const void *userParam) {
    if (type != 33361) {
        std::cout << "[GL_ERROR] (" << type << ")::" << message <<
                  std::endl;
    }
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

// Clears the screen
void Renderer::Clear() const {
    glClear(GL_COLOR_BUFFER_BIT);
}
