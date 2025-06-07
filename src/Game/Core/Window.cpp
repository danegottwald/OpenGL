
#include "Window.h"
#include "../../Input/Input.h"

#include "GUIManager.h"

Window::Window( const WindowData& winData ) :
   m_WindowData( winData )
{}

Window::~Window()
{
   glfwDestroyWindow( m_Window );
   glfwTerminate();
}

Window& Window::Get()
{
   static Window instance = Window( WindowData { "OpenGL" } );
   return instance;
}

GLFWwindow* Window::GetNativeWindow()
{
   return Window::Get().m_Window;
}

void Window::Init()
{
   assert( !m_fRunning && L"Window is already running" );

   // Initialize GLFW
   if( !glfwInit() )
      throw std::runtime_error( "Failed to initialize GLFW" );

   glfwSetErrorCallback( []( int code, const char* message ) { std::cout << "[GLFW Error] (" << code << "): " << message << std::endl; } );

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
   m_Window = glfwCreateWindow( m_WindowData.Width, m_WindowData.Height, m_WindowData.Title.c_str(), nullptr, nullptr );
   if( !m_Window )
      throw std::runtime_error( "Failed to create GLFW window" );

   // Set the window's context and user pointer
   glfwMakeContextCurrent( m_Window );         // Make the window's context current
   glfwSetWindowUserPointer( m_Window, this ); // Set the user pointer to this class instance
   glfwSwapInterval( m_WindowData.VSync );     // Enable VSync

   // Initialize OpenGL functions using GLAD
   if( !gladLoadGL() )
      throw std::runtime_error( "Failed to initialize OpenGL context" );

#ifndef NDEBUG
   std::cout << "GPU Vendor: " << glGetString( GL_VENDOR ) << std::endl;
   std::cout << "GPU Renderer: " << glGetString( GL_RENDERER ) << std::endl;
   std::cout << "OpenGL Version: " << glGetString( GL_VERSION ) << std::endl;
   std::cout << "GLSL Version: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << std::endl;
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

      glShadeModel( GL_SMOOTH );              // Default shading model is smooth
      glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ); // Default clear color is black
   }

   // Set up necessary callbacks for input handling and window events
   SetCallbacks();

   m_fRunning = true; // Window is now running and fully initialized
}

void Window::OnUpdate() const
{
   //Renderer::Get().Draw();

   // Poll for and process events
   glfwPollEvents();

   // Swap front and back buffers
   glfwSwapBuffers( m_Window );
}

glm::vec2 Window::GetMousePosition() const
{
   double xPos, yPos;
   glfwGetCursorPos( Window::GetNativeWindow(), &xPos, &yPos );
   return glm::vec2( xPos, yPos );
}

// --------------------------------------------------------------------
//      Window Queries
// --------------------------------------------------------------------
bool Window::FMinimized() const noexcept
{
   return m_fMinimized;
}

bool Window::IsOpen() const noexcept
{
   return m_fRunning;
}

// --------------------------------------------------------------------
//      WindowData
// --------------------------------------------------------------------
WindowData& Window::GetWindowData()
{
   return m_WindowData;
}

void Window::SetVSync( bool state )
{
   if( state != m_WindowData.VSync )
      glfwSwapInterval( m_WindowData.VSync = state );
}

void Window::SetCallbacks()
{
   m_eventSubscriber.Subscribe< Events::WindowCloseEvent >( [ & ]( const Events::WindowCloseEvent& /*event*/ ) noexcept { m_fRunning = false; } );

   m_eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ & ]( const Events::WindowResizeEvent& event ) noexcept
   {
      m_WindowData.Width  = event.GetWidth();
      m_WindowData.Height = event.GetHeight();
      glViewport( 0, 0, m_WindowData.Width, m_WindowData.Height );
   } );

   m_eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& event ) noexcept
   {
      switch( event.GetKeyCode() )
      {
         case Input::F1:
         {
            m_fRunning = false;
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
            glfwSetInputMode( m_Window, GLFW_CURSOR, fFlip );
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
         std::cout << "[OpenGL " << sourceStr << "] "
                   << "[" << typeStr << "] "
                   << "[" << severityStr << "] "
                   << "[" << id << "]: " << message << std::endl;

         if( severity == GL_DEBUG_SEVERITY_HIGH )
            std::cerr << "CRITICAL: OpenGL error, severity HIGH!" << std::endl;
      }
   },
      nullptr );
#endif

   // GLFW Window Callbacks
   glfwSetWindowSizeCallback( m_Window, []( GLFWwindow* window, int width, int height ) { Events::Dispatch< Events::WindowResizeEvent >( width, height ); } );
   glfwSetWindowCloseCallback( m_Window, []( GLFWwindow* window ) { Events::Dispatch< Events::WindowCloseEvent >(); } );

   // GLFW Key Callbacks
   glfwSetKeyCallback( m_Window,
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

   glfwSetCharCallback( m_Window,
                        []( GLFWwindow* window, unsigned int keycode )
   { Events::Dispatch< Events::KeyTypedEvent >( static_cast< Input::KeyCode >( keycode ) ); } );

   glfwSetMouseButtonCallback( m_Window,
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

   glfwSetScrollCallback( m_Window,
                          []( GLFWwindow* window, double xOffset, double yOffset )
   {
      if( ImGui::GetIO().WantCaptureMouse ) // Block scroll if ImGui has mouse capture active
         return;

      Events::Dispatch< Events::MouseScrolledEvent >( static_cast< float >( xOffset ), static_cast< float >( yOffset ) );
   } );

   glfwSetCursorPosCallback( m_Window,
                             []( GLFWwindow* window, double xPos, double yPos )
   { Events::Dispatch< Events::MouseMovedEvent >( static_cast< float >( xPos ), static_cast< float >( yPos ) ); } );
}
