#pragma once

#include "VertexBufferLayout.h"

class VertexBuffer
{
public:
   VertexBuffer() = default;
   ~VertexBuffer()
   {
      if( m_bufferID )
         glDeleteBuffers( 1, &m_bufferID );
   }

   // Initialize method to set up buffer
   template< typename TContainer >
   void SetBufferData( const TContainer& vertexData, VertexBufferLayout&& layout );
   void Bind() const { glBindBuffer( GL_ARRAY_BUFFER, m_bufferID ); }
   void Unbind() const { glBindBuffer( GL_ARRAY_BUFFER, 0 ); }

   const std::vector< float >& GetVertices() const { return m_vertexData; }

   // Delete Copy and Move Constructors
   VertexBuffer( const VertexBuffer& )            = delete;
   VertexBuffer& operator=( const VertexBuffer& ) = delete;

private:
   unsigned int         m_bufferID { 0 };
   VertexBufferLayout   m_layout;
   std::vector< float > m_vertexData; // Store vertices for ray intersection
};

template< typename TContainer >
void VertexBuffer::SetBufferData( const TContainer& vertexData, VertexBufferLayout&& layout )
{
   m_layout = std::move( layout );
   m_vertexData.assign( vertexData.begin(), vertexData.end() ); // Store data for ray casting

   // Delete previous buffer if exists
   if( m_bufferID )
      glDeleteBuffers( 1, &m_bufferID );

   // Generate and bind buffer
   glGenBuffers( 1, &m_bufferID );
   glBindBuffer( GL_ARRAY_BUFFER, m_bufferID );
   glBufferData( GL_ARRAY_BUFFER, vertexData.size() * sizeof( TContainer::value_type ), vertexData.data(), GL_STATIC_DRAW );

   // Apply layout
   const std::vector< VertexBufferElement >& elements = m_layout.GetElements();
   for( unsigned int i = 0; i < elements.size(); i++ )
   {
      glEnableVertexAttribArray( i );
      const VertexBufferElement& element = elements[ i ];
      glVertexAttribPointer( i, element.m_count, element.m_type, element.m_normalized, m_layout.GetStride(), ( const void* )element.m_offset );
   }

   glBindBuffer( GL_ARRAY_BUFFER, 0 ); // Unbind after setup
}
