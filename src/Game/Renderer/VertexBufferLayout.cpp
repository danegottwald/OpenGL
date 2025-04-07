
#include "VertexBufferLayout.h"

// Returns a string that represents the format of the current Vertex Buffer Layout
std::string VertexBufferLayout::GetVertexFormat()
{
   std::string format;
   for( unsigned int i = 0; i < m_elements.size(); i++ )
   {
      const VertexBufferElement& element = m_elements[ i ];
      format += std::to_string( element.m_count );
      if( i < m_elements.size() - 1 )
         format += "_";
   }

   return format;
}
