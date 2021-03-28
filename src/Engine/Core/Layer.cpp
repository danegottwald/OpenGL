
#include "Layer.h"

#include <array>

#include "GLFW/glfw3.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

#include "Window.h"
#include "PlayerController.h"

Layer::Layer() : m_VAID(0), m_VBID(0), m_IBID(0) {
    glCreateVertexArrays(1, &m_VAID);
    glBindVertexArray(m_VAID);

    glCreateBuffers(1, &m_VBID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBID);

    m_Shader = new Shader("Basic.glsl");
}

Layer::~Layer() {
    glDeleteVertexArrays(1, &m_VAID);
    glDeleteBuffers(1, &m_VBID);
    glDeleteBuffers(1, &m_IBID);
}

void Layer::Enable() {
    // '1000' is how many Vertex structs to allocate space for
    glBufferData(GL_ARRAY_BUFFER, 1000, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (const void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (const void *)12);

    // Continue for each attribute in Vertex Struct
    // ...

    // generate index buffer in the format depending on how many triangles are wanted

    std::array<unsigned int, 18> indices = {
        0, 1, 2,
        2, 3, 1,
        0, 1, 4,
        3, 1, 4,
        2, 3, 4,
        0, 2, 4
    };

    glCreateBuffers(1, &m_IBID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    //load textures
    
}

void Layer::Draw(float frametime) {
    // update camera
    auto &pc = PlayerController::GetController();
    pc.Update(frametime);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // get vertex data here
    std::array<float, 35> vertices = {
        -1.0f, -1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_VBID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    m_Shader->Bind();
    m_Shader->SetUniformMat4f("u_MVP", pc.GetProjectionMatrix() * pc.GetViewMatrix());

    //glBindTextureUnit();

    glBindVertexArray(m_VAID);
    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, nullptr);

    // maybe use below
    //glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
