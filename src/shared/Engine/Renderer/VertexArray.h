#pragma once

class VertexArray
{
public:
   VertexArray() = default;
   ~VertexArray()
   {
      if( m_vertexArrayID )
         glDeleteVertexArrays( 1, &m_vertexArrayID );
   }

   void Initialize() { glCreateVertexArrays( 1, &m_vertexArrayID ); }
   void Bind() const { glBindVertexArray( m_vertexArrayID ); }
   void Unbind() const { glBindVertexArray( 0 ); }

   // Delete Copy and Move Constructors
   VertexArray( const VertexArray& )            = delete;
   VertexArray& operator=( const VertexArray& ) = delete;

private:
   unsigned int m_vertexArrayID { 0 };
};
