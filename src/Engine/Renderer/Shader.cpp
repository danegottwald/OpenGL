#include "Shader.h"


// ----------------------------------------------------------------
// Shader
// ----------------------------------------------------------------
Shader::Shader( InitType type, std::string_view vertex, std::string_view fragment ) :
   m_source( GetShaderProgramSource( type, vertex, fragment ) ),
   m_rendererId( CreateShader() )
{}

// Frees memory and restores the name taken by m_RendererID
Shader::~Shader()
{
   glDeleteProgram( m_rendererId );
}


void Shader::Bind() const
{
   glUseProgram( m_rendererId ); // Set the current active shader program
}


void Shader::Unbind() const
{
   glUseProgram( 0 ); // Unbinds the shader program currently active
}


Shader::ShaderProgramSource Shader::GetShaderProgramSource( InitType type, std::string_view vertex, std::string_view fragment )
{
   switch( type )
   {
      case InitType::SOURCE: return { std::string( vertex ), std::string( fragment ) };

      case InitType::FILE:
      {
         auto readFileFn = []( const std::string& path ) -> std::string
         {
            std::ifstream file( std::string( path ), std::ios::in | std::ios::binary );
            if( !file )
               throw std::runtime_error( "Failed to open shader file: " + std::string( path ) );

            std::string content;
            file.seekg( 0, std::ios::end );
            content.resize( file.tellg() );
            file.seekg( 0, std::ios::beg );
            file.read( &content[ 0 ], content.size() );
            return content;
         };
         return { readFileFn( std::format( "./res/shaders/{}", vertex ) ), readFileFn( std::format( "./res/shaders/{}", fragment ) ) };
      }

      default: throw std::runtime_error( "Shader::GetShaderProgramSource - Unhandled InitType" );
   }
}


// Creates a Shader program, attaches a vertex and fragment shader, then links
// and validates the shader. Returns the Shader program's id
unsigned int Shader::CreateShader()
{
   // Creates an empty program object for which shader objects can be attached
   unsigned int programID = glCreateProgram();

   // Creates shader objects for the Vertex and Fragment shaders
   unsigned int vs = CompileShader( GL_VERTEX_SHADER, m_source.vertex );
   unsigned int fs = CompileShader( GL_FRAGMENT_SHADER, m_source.fragment );

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
unsigned int Shader::CompileShader( unsigned int type, std::string_view source )
{
   // Set the shader's (id) source code and compile it
   const char*  src      = source.data();
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


// Returns the location of a uniform value within the Shader
int Shader::GetUniformLocation( const std::string_view& name )
{
   // Search the cache to check if the uniform variable has been hit before
   if( m_uniformLocationCache.find( name ) != m_uniformLocationCache.end() )
      return m_uniformLocationCache[ name ];

   // Find the location of the uniform variable
   int location = glGetUniformLocation( m_rendererId, name.data() );
   if( location == -1 )
      std::println( std::cerr, "Warning: uniform '{}' doesn't exist!", name );

   // Cache variable for future references
   m_uniformLocationCache[ name ] = location;
   return location;
}
