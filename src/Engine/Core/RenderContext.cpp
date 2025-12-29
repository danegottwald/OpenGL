#include "RenderContext.h"


RenderContext::RenderContext( GLFWwindow& window ) noexcept :
   m_window( window )
{}


RenderContext::~RenderContext() = default;


void RenderContext::Init( bool fVSync )
{
   glfwMakeContextCurrent( &m_window );
   glfwSwapInterval( fVSync );

   if( !gladLoadGL() )
      throw std::runtime_error( "Failed to initialize OpenGL context" );

#ifndef NDEBUG
   std::println( "GPU Vendor: {}", reinterpret_cast< const char* >( glGetString( GL_VENDOR ) ) );
   std::println( "GPU Renderer: {}", reinterpret_cast< const char* >( glGetString( GL_RENDERER ) ) );
   std::println( "OpenGL Version: {}", reinterpret_cast< const char* >( glGetString( GL_VERSION ) ) );
   std::println( "GLSL Version: {}", reinterpret_cast< const char* >( glGetString( GL_SHADING_LANGUAGE_VERSION ) ) );
#endif

   {                             // ---- OpenGL State Setup ----
      glEnable( GL_DEPTH_TEST ); // Enable depth testing
      glDepthFunc( GL_LESS );    // Accept fragment if it closer to the camera than the former one

      glEnable( GL_CULL_FACE ); // Enable face culling
      glCullFace( GL_BACK );    // Cull back-facing polygons
      glFrontFace( GL_CCW );    // Counter-clockwise winding is considered front-facing

      glEnable( GL_BLEND );                                // Enable blending
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // Standard alpha blending

      glShadeModel( GL_SMOOTH ); // Default shading model is smooth

      // Clear color to cornflower blue
      glClearColor( 0x64 / 255.0f, 0x93 / 255.0f, 0xED / 255.0f, 1.0f );
   }

   SetCallbacks();
}


void RenderContext::Present()
{
   glfwSwapBuffers( &m_window );
}


void RenderContext::SetVSync( bool fState )
{
   glfwSwapInterval( fState );
}


void RenderContext::UpdateViewport( int x, int y, int width, int height )
{
   glViewport( x, y, width, height );
}


void RenderContext::SetCallbacks()
{
#ifndef NDEBUG
   glEnable( GL_DEBUG_OUTPUT );
   glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
   glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE );
   glDebugMessageCallback(
      []( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/ )
   {
      const char* sourceStr = nullptr;
      switch( source )
      {
         case GL_DEBUG_SOURCE_API:             sourceStr = "API"; break;
         case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System"; break;
         case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
         case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party"; break;
         case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application"; break;
         case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other"; break;
         default:                              sourceStr = "Unknown"; break;
      }

      const char* typeStr = nullptr;
      switch( type )
      {
         case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
         case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behavior"; break;
         case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behavior"; break;
         case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
         case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
         case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
         case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
         case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
         default:                                typeStr = "Unknown"; break;
      }

      const char* severityStr = nullptr;
      switch( severity )
      {
         case GL_DEBUG_SEVERITY_HIGH:         severityStr = "High"; break;
         case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "Medium"; break;
         case GL_DEBUG_SEVERITY_LOW:          severityStr = "Low"; break;
         case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Notification"; break;
         default:                             severityStr = "Unknown"; break;
      }

      if( severity != GL_DEBUG_SEVERITY_NOTIFICATION )
      {
         std::println( "[OpenGL {}] [{}] [{}] [{}]: {}", sourceStr, typeStr, severityStr, id, message );

         if( severity == GL_DEBUG_SEVERITY_HIGH )
            std::println( std::cerr, "CRITICAL: OpenGL error, severity HIGH!" );
      }
   },
      nullptr );
#endif
}
