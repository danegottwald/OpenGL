#pragma once

class VertexBuffer
{
private:
   // Member Variables
   unsigned int m_RendererID;

public:
   // Constructors
   VertexBuffer( const void* data, unsigned int size );
   // Overload for std::array use
   template< typename T, size_t N >
   explicit VertexBuffer( const std::array< T, N >& vb );

   // Destructor
   ~VertexBuffer();

   // Member Functions
   void Bind() const;
   void Unbind() const;
};

// Constructor template for std::array
template< typename T, size_t N >
VertexBuffer::VertexBuffer( const std::array< T, N >& vb )
{
   // Generates 1 buffer object name (ID) into m_RendererID
   glGenBuffers( 1, &m_RendererID );
   // Binds the Vertex Buffer (ID/m_RendererID) to the OpenGL state
   glBindBuffer( GL_ARRAY_BUFFER, m_RendererID );
   // Creates/initializes the bound Vertex Buffer with data of size 'size'
   glBufferData( GL_ARRAY_BUFFER, vb.size() * sizeof( vb.front() ), vb.data(), GL_STATIC_DRAW );
}
