#include "Window.h"

// Local dependencies
#include "UI.h"

// Project dependencies
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Input/Input.h>


// --------------------------------------------------------------------
// Window
// --------------------------------------------------------------------
Window::Window( std::string_view title ) noexcept
{
   m_state.title = title;
}


Window::~Window()
{
   UI::UIBuffer::Shutdown();

   glfwDestroyWindow( m_window );
   glfwTerminate();
}


Window& Window::Get()
{
   static Window instance = Window( "OpenGL" );
   return instance;
}


GLFWwindow* Window::GetNativeWindow()
{
   return Window::Get().m_window;
}


void Window::Init()
{
   // Initialize GLFW
   if( !glfwInit() )
      throw std::runtime_error( "Failed to initialize GLFW" );

   glfwSetErrorCallback( []( int code, const char* message ) { std::println( std::cerr, "[GLFW Error] ({}) {}", code, message ); } );

#ifndef NDEBUG
   std::clog << "GLFW DEBUG CONTEXT ENABLED" << std::endl;
   glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );
#endif

   // Set OpenGL Version and Profile
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );                 // OpenGL major version 4
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );                 // OpenGL minor version 3
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE ); // Use core profile (no deprecated functions)

   glfwWindowHint( GLFW_DEPTH_BITS, 24 ); // Set the depth buffer to 24 bits
   glfwWindowHint( GLFW_SAMPLES, 4 );     // Enable Multi-Sample Anti-Aliasing (MSAA)

   // Create the GLFW window
   m_window = glfwCreateWindow( m_state.size.x, m_state.size.y, m_state.title.c_str(), nullptr, nullptr );
   if( !m_window )
      throw std::runtime_error( "Failed to create GLFW window" );

   // Set the window's context and user pointer
   glfwMakeContextCurrent( m_window );         // Make the window's context current
   glfwSetWindowUserPointer( m_window, this ); // Set the user pointer to this class instance
   glfwSwapInterval( m_state.fVSync );         // Enable VSync

   // Initialize OpenGL functions using GLAD
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

      //glfwWindowHint( GLFW_SAMPLES, 4 ); // Enable Multi-Sample Anti-Aliasing (MSAA)
      //glEnable( GL_MULTISAMPLE );        // Enable MSAA

      glEnable( GL_CULL_FACE ); // Enable face culling
      glCullFace( GL_BACK );    // Cull back-facing polygons
      glFrontFace( GL_CCW );    // Counter-clockwise winding is considered front-facing

      glEnable( GL_BLEND );                                // Enable blending
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // Standard alpha blending

      glShadeModel( GL_SMOOTH ); // Default shading model is smooth

      // Clear color to cornflower blue
      const float r = 100.0f / 255.0f, g = 147.0f / 255.0f, b = 237.0f / 255.0f;
      glClearColor( r, g, b, 1.0f );
   }

   // Set up necessary callbacks for input handling and window events
   SetCallbacks();

   UI::UIBuffer::Init();
}


bool Window::FProcessEvents()
{
   if( glfwWindowShouldClose( m_window ) )
      m_state.fShouldClose = true;

   if( m_state.fShouldClose )
   {
      glfwSetWindowShouldClose( m_window, GLFW_TRUE );
      return false; // early out if window is closing
   }

   glfwPollEvents(); // poll glfw events

   double x, y;
   glfwGetCursorPos( m_window, &x, &y );
   glm::vec2 currentPos = { ( float )x, ( float )y };

   const bool fCaptured   = ( glfwGetInputMode( m_window, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED );
   m_state.mouseDelta     = ( fCaptured && !m_state.fMouseCaptured ) ? glm::vec2( 0.0f ) : currentPos - m_state.mousePos;
   m_state.mousePos       = currentPos;
   m_state.fMouseCaptured = fCaptured;

   return true;
}


void Window::Present()
{
   glfwSwapBuffers( m_window ); // swap front and back buffers
}


void Window::SetVSync( bool fState )
{
   if( fState != m_state.fVSync )
   {
      m_state.fVSync = fState;
      glfwSwapInterval( fState );
   }
}


void Window::SetCallbacks()
{
   m_eventSubscriber.Subscribe< Events::WindowCloseEvent >( [ & ]( const Events::WindowCloseEvent& /*event*/ ) noexcept { m_state.fShouldClose = true; } );

   m_eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& event ) noexcept
   {
      switch( event.GetKeyCode() )
      {
         case Input::F1:
         {
            m_state.fShouldClose = true;
            break;
         }

         case Input::P:
         {
            static int polygonModeFlip = GL_FILL;
            polygonModeFlip            = ( polygonModeFlip == GL_FILL ) ? GL_LINE : GL_FILL;
            glPolygonMode( GL_FRONT_AND_BACK, polygonModeFlip );
            break;
         }

         case Input::Escape:
         {
            static int fFlip = GLFW_CURSOR_NORMAL;
            fFlip            = ( fFlip == GLFW_CURSOR_NORMAL ) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
            glfwSetInputMode( m_window, GLFW_CURSOR, fFlip );

            int width, height;
            glfwGetWindowSize( m_window, &width, &height );
            glfwSetCursorPos( m_window, width * 0.5f, height * 0.5f ); // Center the cursor
            break;
         }

         case Input::F11:
         {
            static bool fFullscreen = false;
            fFullscreen             = !fFullscreen;

            GLFWmonitor*       primary = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode    = glfwGetVideoMode( primary );
            if( fFullscreen )
            {
               glfwSetWindowAttrib( m_window, GLFW_AUTO_ICONIFY, GLFW_FALSE );
               glfwSetWindowMonitor( m_window, primary, 0, 0, mode->width, mode->height, mode->refreshRate );
            }
            else
            {
               const float paddingPercent = 0.1f; // 10% padding
               int         padX           = static_cast< int >( mode->width * paddingPercent );
               int         padY           = static_cast< int >( mode->height * paddingPercent );
               int         windowedWidth = mode->width - padX, windowedHeight = mode->height - padY;
               int         windowedX = padX / 2, windowedY = padY / 2;
               glfwSetWindowMonitor( m_window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0 );
            }
            break;
         }

         default: break;
      }
   } );

#ifndef NDEBUG
   // OpenGL Error Callback (API RELATED, RELOCATE)
   glEnable( GL_DEBUG_OUTPUT );
   glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS ); // enable only on debug
   glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE );
   glDebugMessageCallback(
      []( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam )
   {
      // Determine the source
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

      // Determine the type
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

      // Determine the severity
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
         // Log the message with all relevant details
         std::println( "[OpenGL {}] [{}] [{}] [{}]: {}", sourceStr, typeStr, severityStr, id, message );

         if( severity == GL_DEBUG_SEVERITY_HIGH )
            std::println( std::cerr, "CRITICAL: OpenGL error, severity HIGH!" );
      }
   },
      nullptr );
#endif

   // GLFW Window Callbacks
   glfwSetWindowCloseCallback( m_window, []( GLFWwindow* window ) { Events::Dispatch< Events::WindowCloseEvent >(); } );

   glfwSetWindowIconifyCallback( m_window,
                                 []( GLFWwindow* window, int iconified )
   {
      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
         pWindow->GetWindowState().fMinimized = ( iconified == GLFW_TRUE );
   } );

   glfwSetFramebufferSizeCallback( m_window,
                                   []( GLFWwindow* window, int width, int height )
   {
      if( width == 0 || height == 0 )
         return; // Ignore "fake" resizes during minimization

      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
         pWindow->GetWindowState().size = { static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) };

      glViewport( 0, 0, width, height ); // EVENTUALLY MOVE TO SOME RENDERER SPECIFIC LOCATION
      Events::Dispatch< Events::WindowResizeEvent >( width, height );
   } );

   glfwSetWindowFocusCallback( m_window,
                               []( GLFWwindow* window, int fFocused )
   {
      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
         pWindow->OnFocusChanged( static_cast< bool >( fFocused ) );
   } );

   // GLFW Key Callbacks
   glfwSetKeyCallback( m_window,
                       []( GLFWwindow* window, int key, int scancode, int action, int mods )
   {
      if( ImGui::GetIO().WantTextInput ) // Block input if ImGui has text input active
         return;

      Input::KeyCode keyCode = static_cast< Input::KeyCode >( key );
      if( action != GLFW_REPEAT )
         Input::SetKeyPressed( keyCode, static_cast< bool >( action ) ); // Update input manager state

      // Dispatch events
      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::KeyPressedEvent >( keyCode, false /* fRepeated */ ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::KeyReleasedEvent >( keyCode ); break;
         case GLFW_REPEAT:  Events::Dispatch< Events::KeyPressedEvent >( keyCode, true /* fRepeated */ ); break;
         default:           throw std::runtime_error( "Unknown KeyEvent" );
      }
   } );

   glfwSetCharCallback( m_window,
                        []( GLFWwindow* window, unsigned int keycode )
   { Events::Dispatch< Events::KeyTypedEvent >( static_cast< Input::KeyCode >( keycode ) ); } );

   glfwSetMouseButtonCallback( m_window,
                               []( GLFWwindow* window, int button, int action, int mods )
   {
      if( ImGui::GetIO().WantCaptureMouse ) // Block mouse if ImGui has mouse capture active
         return;

      Input::MouseCode mouseCode = static_cast< Input::MouseCode >( button );
      Input::SetMouseButtonPressed( mouseCode, action != GLFW_RELEASE ); // Update input manager state

      // Dispatch events
      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::MouseButtonPressedEvent >( mouseCode ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::MouseButtonReleasedEvent >( mouseCode ); break;
         default:           throw std::runtime_error( "Unknown MouseButtonEvent" );
      }
   } );

   glfwSetScrollCallback( m_window,
                          []( GLFWwindow* window, double xOffset, double yOffset )
   {
      if( ImGui::GetIO().WantCaptureMouse ) // Block scroll if ImGui has mouse capture active
         return;

      Events::Dispatch< Events::MouseScrolledEvent >( static_cast< float >( xOffset ), static_cast< float >( yOffset ) );
   } );
}
