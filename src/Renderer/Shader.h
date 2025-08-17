#pragma once

class Shader
{
private:
   // Holds the separate source codes for each Shader
   struct ShaderProgramSource
   {
      std::string VertexSource;
      std::string FragmentSource;
   };

   // Member Variables
   unsigned int                                         m_RendererID;
   ShaderProgramSource                                  m_Source;
   std::unordered_map< std::string_view, unsigned int > m_UniformLocationCache;

   // Member Functions
   ShaderProgramSource ParseShader( const std::string& vertSrc, const std::string& fragSrc );
   unsigned int        CreateShader( const std::string& vertexShader, const std::string& fragmentShader );
   unsigned int        CompileShader( unsigned int type, const std::string& source );

   int GetUniformLocation( const std::string_view& name );

public:
   // Constructor & Destructor
   Shader();
   ~Shader();

   // Member Functions
   void Bind() const;
   void Unbind() const;

   // Uniform Manipulations - Only supports int, float, glm::mat3, glm::mat4
   template< typename... Args >
   void SetUniform( const std::string_view& name, const Args&... args )
   {
      static_assert( sizeof...( args ) <= 4, "Unsupported uniform parameter pack size [1-4]." );

      const GLint location = GetUniformLocation( name );
      if constexpr( ( std::conjunction_v< std::is_same< Args, int >... > )) // Handle integers
      {
         if constexpr( sizeof...( args ) == 1 )
            glUniform1i( location, args... );
         else if constexpr( sizeof...( args ) == 2 )
            glUniform2i( location, args... );
         else if constexpr( sizeof...( args ) == 3 )
            glUniform3i( location, args... );
         else if constexpr( sizeof...( args ) == 4 )
            glUniform4i( location, args... );
      }
      else if constexpr( ( std::conjunction_v< std::is_same< Args, float >... > )) // Handle floats
      {
         if constexpr( sizeof...( args ) == 1 )
            glUniform1f( location, args... );
         else if constexpr( sizeof...( args ) == 2 )
            glUniform2f( location, args... );
         else if constexpr( sizeof...( args ) == 3 )
            glUniform3f( location, args... );
         else if constexpr( sizeof...( args ) == 4 )
            glUniform4f( location, args... );
      }
      else if constexpr( sizeof...( args ) == 1 && std::is_same_v< Args..., glm::mat4 > )
         glUniformMatrix4fv( location, 1, GL_FALSE, glm::value_ptr( args... ) ); // Handle glm::mat4
      else if constexpr( sizeof...( args ) == 1 && std::is_same_v< Args..., glm::mat3 > )
         glUniformMatrix3fv( location, 1, GL_FALSE, glm::value_ptr( args... ) ); // Handle glm::mat3
      else if constexpr( sizeof...( args ) == 1 && std::is_same_v< Args..., glm::vec2 > )
         glUniform2fv( location, 1, glm::value_ptr( args... ) ); // Handle glm::vec2
      else if constexpr( sizeof...( args ) == 1 && std::is_same_v< Args..., glm::vec3 > )
         glUniform3fv( location, 1, glm::value_ptr( args... ) ); // Handle glm::vec3
      else if constexpr( sizeof...( args ) == 1 && std::is_same_v< Args..., glm::vec4 > )
         glUniform4fv( location, 1, glm::value_ptr( args... ) ); // Handle glm::vec4
      else
         static_assert( false, "Unsupported uniform parameter pack type [int, float, glm::mat3, glm::mat4, glm::vec2, glm::vec3, glm::vec4]." );
   }
};
