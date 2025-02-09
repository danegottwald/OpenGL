
#include "VertexBufferLayout.h"

// Returns a string that represents the format of the current Vertex Buffer
// Layout
std::string VertexBufferLayout::GetVertexFormat()
{
   std::string format;
   const auto& elements = GetElements();
   for( unsigned int i = 0; i < m_Elements.size(); i++ )
   {
      const auto& element = elements[ i ];
      format += std::to_string( element.count );
      if( i + 1 < m_Elements.size() )
      {
         format += "_";
      }
   }
   return format;
}
