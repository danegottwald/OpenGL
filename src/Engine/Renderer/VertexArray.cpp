
#include "VertexArray.h"

VertexArray::VertexArray() {
    // Generate a Vertex Array object and store its ID
    glGenVertexArrays(1, &m_VertexArrayID);
}

// Delete the Vertex Array object specified by m_RendererID
VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_VertexArrayID);
}

// Binds the Vertex Array object
void VertexArray::Bind() const {
    glBindVertexArray(m_VertexArrayID);
}

// Unbind/break the currently bound Vertex Array
void VertexArray::Unbind() const {
    glBindVertexArray(0);
}

void VertexArray::AddBuffer() {
    glCreateVertexArrays(1, &m_VertexArrayID);
    glBindVertexArray(m_VertexArrayID);

    glCreateBuffers(1, &m_VertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    // sizeof(m_Data) * 1, the 1 is the number of Vertex's
    // glBufferData(GL_ARRAY_BUFFER, sizeof(m_Data) * 3, nullptr,
    // GL_DYNAMIC_DRAW);

    glEnableVertexArrayAttrib(m_VertexBufferID, 0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const
    // void*)offsetof(Vertex, Position));

    glEnableVertexArrayAttrib(m_VertexBufferID, 1);
    // glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const
    // void*)offsetof(Vertex, Color));

    // Continue for each attribute in Vertex Struct
    // ...

    // define indices

    // glCreateBuffers(1, &m_IndexBufferID);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
    // GL_STATIC_DRAW);

    // load textures

    // OnUpdate()

    // get vertex data here

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glUseProgram(shader);
    // glBindTextureUnit();

    // get camera controller
    // set view projection and transform from camera

    glBindVertexArray(m_VertexArrayID);
    // glDrawElements(GL_TRIANGLES, ib.Count(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}
