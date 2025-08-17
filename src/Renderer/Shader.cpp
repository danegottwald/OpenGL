#include "Shader.h"

// Create a attach the Shaders within the specified file
Shader::Shader()
{
   // Parse the Shader file into separate vertex and fragment shaders
   m_Source = ParseShader( "basic_vert.glsl", "basic_frag.glsl" );

   // Returns an ID to reference the current Shader program
   m_RendererID = CreateShader( m_Source.VertexSource, m_Source.FragmentSource );
}

// Frees memory and restores the name taken by m_RendererID
Shader::~Shader()
{
   glDeleteProgram( m_RendererID );
}

// Sets the Shader program as part of the current rendering state
void Shader::Bind() const
{
   glUseProgram( m_RendererID );
}

// Removes the Shader program from part of the rendering state
void Shader::Unbind() const
{
   glUseProgram( 0 );
}

// Parses the specified file into separate Shader source code
// Returns a struct that contains the source codes
Shader::ShaderProgramSource Shader::ParseShader( const std::string& vertSrc, const std::string& fragSrc )
{
   std::string       line;
   std::stringstream ss[ 2 ];

   std::ifstream vertFileStream( "./res/shaders/" + vertSrc );
   while( getline( vertFileStream, line ) )
      ss[ 0 ] << line << '\n';

   std::ifstream fragFileStream( "./res/shaders/" + fragSrc );
   while( getline( fragFileStream, line ) )
      ss[ 1 ] << line << '\n';

   return { ss[ 0 ].str(), ss[ 1 ].str() };
}

// Creates a Shader program, attaches a vertex and fragment shader, then links
// and validates the shader. Returns the Shader program's id
unsigned int Shader::CreateShader( const std::string& vertexShader, const std::string& fragmentShader )
{
   // Creates an empty program object for which shader objects can be attached
   unsigned int programID = glCreateProgram();

   // Creates shader objects for the Vertex and Fragment shaders
   unsigned int vs = CompileShader( GL_VERTEX_SHADER, vertexShader );
   unsigned int fs = CompileShader( GL_FRAGMENT_SHADER, fragmentShader );

   // Attaches the shader objects to the program object
   glAttachShader( programID, vs );
   glAttachShader( programID, fs );

   // Links the program object and creates executables for each of the shaders
   glLinkProgram( programID );

   int success;
   glGetProgramiv( programID, GL_LINK_STATUS, &success );
   if( success == GL_FALSE )
   {
      int length;
      glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &length );

      std::vector< char > message( length );
      glGetProgramInfoLog( programID, length, &length, message.data() );
      std::println( std::cerr, "Failed to link shader program id {}!", programID );
      std::println( std::cerr, "{}", message.data() );
      glDeleteProgram( programID );
      return 0;
   }

   // Checks/validates whether the executables contained in 'program' can execute
   glValidateProgram( programID );

   glGetProgramiv( programID, GL_VALIDATE_STATUS, &success );
   if( success == GL_FALSE )
   {
      int length;
      glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &length );
      std::vector< char > message( length );
      glGetProgramInfoLog( programID, length, &length, message.data() );
      std::println( std::cerr, "Failed to validate shader program id {}!", programID );
      std::println( std::cerr, "{}", message.data() );
      glDeleteProgram( programID );
      return 0;
   }

   // Flags the shaders for deletion, but will not be deleted until it is no
   // longer attached to any program object.  This will happen when
   // glDeleteProgram() is called on 'program'
   glDeleteShader( vs );
   glDeleteShader( fs );

   // Returns the id for which the program object can be referenced
   return programID;
}

// Compiles the Shader's source code
// Returns a Shader's id for attaching to a Shader program
unsigned int Shader::CompileShader( unsigned int type, const std::string& source )
{
   // Set the shader's (id) source code and compile it
   const char*  src      = source.c_str();
   unsigned int shaderID = glCreateShader( type );
   glShaderSource( shaderID, 1, &src, nullptr );
   glCompileShader( shaderID );

   // Error Handling
   int success;
   glGetShaderiv( shaderID, GL_COMPILE_STATUS, &success );
   if( success == GL_FALSE )
   {
      int length;
      glGetShaderiv( shaderID, GL_INFO_LOG_LENGTH, &length );
      std::vector< char > message( length );
      glGetShaderInfoLog( shaderID, length, &length, message.data() );
      std::println( std::cerr, "Failed to compile {} shader id {}!", ( type == GL_VERTEX_SHADER ? "vertex" : "fragment" ), shaderID );
      std::println( std::cerr, "{}", message.data() );
      glDeleteShader( shaderID );
      return 0;
   }

   return shaderID;
}

/* Uniform Related Functions */

// Returns the location of a uniform value within the Shader
int Shader::GetUniformLocation( const std::string_view& name )
{
   // Search the cache to check if the uniform variable has been hit before
   if( m_UniformLocationCache.find( name ) != m_UniformLocationCache.end() )
      return m_UniformLocationCache[ name ];

   // Find the location of the uniform variable
   int location = glGetUniformLocation( m_RendererID, name.data() );
   if( location == -1 )
      std::println( std::cerr, "Warning: uniform '{}' doesn't exist!", name );

   // Cache variable for future references
   m_UniformLocationCache[ name ] = location;
   return location;
}
