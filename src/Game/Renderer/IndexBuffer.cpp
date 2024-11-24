#include "IndexBuffer.h"

// Generate an Index Buffer, bind it, and fill it with the specified data
IndexBuffer::IndexBuffer( const unsigned int* data, unsigned int count )
    : m_Count( count )
{
// Generates 1 buffer object name (ID) into m_RendererID
    glGenBuffers( 1, &m_RendererID );
    // Binds the Index Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_RendererID );
    // Creates/initializes the bound Index Buffer with 'count' indices provided
    // in 'data'
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, count * sizeof( unsigned int ), data,
        GL_STATIC_DRAW );
}

// Deletes the Index Buffer referenced by its ID
IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers( 1, &m_RendererID );
}

// Manually bind the Index Buffer (ID/m_RendererID) to the OpenGL state
void IndexBuffer::Bind() const
{
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_RendererID );
}

// Unbinds the most recently bound Index Buffer
void IndexBuffer::Unbind() const
{
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}
