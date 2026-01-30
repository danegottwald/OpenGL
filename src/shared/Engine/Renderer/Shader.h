#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
   enum InitType
   {
      FILE = 0,
      SOURCE
   };

   struct ShaderProgramSource
   {
      std::string vertex;
      std::string fragment;
   };

   Shader( InitType type, std::string_view vertex, std::string_view fragment );
   ~Shader();

   void Bind() const;
   void Unbind() const;

   template< typename T >
   void SetUniform( const std::string_view& name, const T& value );

   template< typename... Args >
   void SetUniform( const std::string_view& name, Args... args );

private:
   ShaderProgramSource GetShaderProgramSource( InitType type, std::string_view vertex, std::string_view fragment );
   unsigned int        CreateShader();
   unsigned int        CompileShader( unsigned int type, std::string_view source );
   int                 GetUniformLocation( const std::string_view& name );

   ShaderProgramSource                    m_source {};
   unsigned int                           m_rendererId {};
   std::unordered_map< std::string, int > m_uniformLocationCache;
};

template<>
inline void Shader::SetUniform< int >( const std::string_view& name, const int& value )
{
   glUniform1i( GetUniformLocation( name ), value );
}

template<>
inline void Shader::SetUniform< unsigned int >( const std::string_view& name, const unsigned int& value )
{
   glUniform1ui( GetUniformLocation( name ), value );
}

template<>
inline void Shader::SetUniform< float >( const std::string_view& name, const float& value )
{
   glUniform1f( GetUniformLocation( name ), value );
}

template<>
inline void Shader::SetUniform< glm::vec2 >( const std::string_view& name, const glm::vec2& value )
{
   glUniform2f( GetUniformLocation( name ), value.x, value.y );
}

template<>
inline void Shader::SetUniform< glm::vec3 >( const std::string_view& name, const glm::vec3& value )
{
   glUniform3f( GetUniformLocation( name ), value.x, value.y, value.z );
}

template<>
inline void Shader::SetUniform< glm::vec4 >( const std::string_view& name, const glm::vec4& value )
{
   glUniform4f( GetUniformLocation( name ), value.x, value.y, value.z, value.w );
}

template<>
inline void Shader::SetUniform< glm::mat3 >( const std::string_view& name, const glm::mat3& value )
{
   glUniformMatrix3fv( GetUniformLocation( name ), 1, GL_FALSE, glm::value_ptr( value ) );
}

template<>
inline void Shader::SetUniform< glm::mat4 >( const std::string_view& name, const glm::mat4& value )
{
   glUniformMatrix4fv( GetUniformLocation( name ), 1, GL_FALSE, glm::value_ptr( value ) );
}

template< typename... Args >
inline void Shader::SetUniform( const std::string_view& name, Args... args )
{
   if constexpr( sizeof...( Args ) == 2 )
      glUniform2f( GetUniformLocation( name ), args... );
   else if constexpr( sizeof...( Args ) == 3 )
      glUniform3f( GetUniformLocation( name ), args... );
   else if constexpr( sizeof...( Args ) == 4 )
      glUniform4f( GetUniformLocation( name ), args... );
}
