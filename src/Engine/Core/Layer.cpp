
#include "Layer.h"

#include <array>

#include "GLFW/glfw3.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

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
    // '6' is how many Vertex structs to allocate space for
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * 4, indices.data(), GL_STATIC_DRAW);

    //load textures
    
}

void Layer::Draw() {
    // update camera
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

    const float radius = 3.0f;
    float camX = sin(glfwGetTime()) * radius;
    float camZ = cos(glfwGetTime()) * radius;
    glm::mat4 view;
    view = glm::lookAt(glm::vec3(camX, 1.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 proj = glm::perspective(90.0f, (float)(16/9), 0.1f, 100.0f);
    glm::mat4 mvp = proj * view;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    m_Shader->Bind();
    m_Shader->SetUniformMat4f("u_MVP", mvp);
    //glBindTextureUnit();

    // get camera controller
    // set view projection and transform from camera

    glBindVertexArray(m_VAID);
    // Issue Draw Call
    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, nullptr);

    // maybe use below
    //glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
