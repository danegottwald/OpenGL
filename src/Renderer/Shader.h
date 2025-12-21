#pragma once

class Shader
{
public:
   enum InitType
   {
      SOURCE = 0,
      FILE   = 1
   };
   Shader( InitType type, std::string_view vertex, std::string_view fragment );
   ~Shader();

   // Member Functions
   void Bind() const;
   void Unbind() const;

   // Uniform Manipulations - Only supports int, float, glm::mat3, glm::mat4
   template< typename... Args >
   void SetUniform( std::string_view name, const Args&... args )
   {
      const GLint loc = GetUniformLocation( name );
      if constexpr( ( std::is_same_v< Args, int > && ... ) ) // int pack
      {
         if constexpr( sizeof...( Args ) == 1 )
            glUniform1i( loc, args... );
         else if constexpr( sizeof...( Args ) == 2 )
            glUniform2i( loc, args... );
         else if constexpr( sizeof...( Args ) == 3 )
            glUniform3i( loc, args... );
         else
            glUniform4i( loc, args... );
      }
      else if constexpr( ( std::is_same_v< Args, float > && ... ) ) // float pack
      {
         if constexpr( sizeof...( Args ) == 1 )
            glUniform1f( loc, args... );
         else if constexpr( sizeof...( Args ) == 2 )
            glUniform2f( loc, args... );
         else if constexpr( sizeof...( Args ) == 3 )
            glUniform3f( loc, args... );
         else
            glUniform4f( loc, args... );
      }
      else if constexpr( sizeof...( Args ) == 1 ) // single glm type
      {
         using T = std::decay_t< Args... >;
         if constexpr( std::is_same_v< T, glm::vec2 > )
            glUniform2fv( loc, 1, glm::value_ptr( args... ) );
         else if constexpr( std::is_same_v< T, glm::vec3 > )
            glUniform3fv( loc, 1, glm::value_ptr( args... ) );
         else if constexpr( std::is_same_v< T, glm::vec4 > )
            glUniform4fv( loc, 1, glm::value_ptr( args... ) );
         else if constexpr( std::is_same_v< T, glm::mat3 > )
            glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( args... ) );
         else if constexpr( std::is_same_v< T, glm::mat4 > )
            glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( args... ) );
         else
            static_assert( false, "Unsupported uniform type" );
      }
      else
         static_assert( false, "Unsupported uniform parameter pack type" );
   }

private:
   struct ShaderProgramSource
   {
      std::string vertex;
      std::string fragment;
   };

   ShaderProgramSource                                  m_source;
   unsigned int                                         m_rendererId;
   std::unordered_map< std::string_view, unsigned int > m_uniformLocationCache;

   ShaderProgramSource GetShaderProgramSource( InitType type, std::string_view vertex, std::string_view fragment );
   unsigned int        CreateShader();
   unsigned int        CompileShader( unsigned int type, std::string_view source );

   int GetUniformLocation( const std::string_view& name );
};
