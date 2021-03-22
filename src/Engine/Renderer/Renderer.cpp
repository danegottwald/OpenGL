
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