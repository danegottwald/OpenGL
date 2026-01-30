#pragma once

class IndexBuffer
{
public:
   IndexBuffer() = default;
   ~IndexBuffer()
   {
      if( m_bufferID )
         glDeleteBuffers( 1, &m_bufferID );
   }

   // Initialize method to create and populate the index buffer
   template< typename TContainer >
   void SetBufferData( const TContainer& indexData )
   {
      m_size = indexData.size();

      // Delete previous buffer if exists
      if( m_bufferID )
         glDeleteBuffers( 1, &m_bufferID );

      glGenBuffers( 1, &m_bufferID );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_bufferID );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_size * sizeof( unsigned int ), indexData.data(), GL_STATIC_DRAW );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ); // Unbind after setup
   }

   void Bind() const { glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_bufferID ); }
   void Unbind() const { glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ); }

   inline unsigned int GetSize() const { return m_size; }

private:
   unsigned int m_bufferID { 0 };
   unsigned int m_size { 0 };
};
