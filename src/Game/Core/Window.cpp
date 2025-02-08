
#include "Window.h"

Window::Window( const WindowData& winData ) :
   m_WindowData( winData )
{
   // Initialize GLFW
   uint32_t success = glfwInit();
   _ASSERT_EXPR( success == GLFW_TRUE, L"Failed to initialize GLFW" );
   glfwSetErrorCallback( []( int code, const char* message ) { std::cout << "[GLFW Error] (" << code << "): " << message << std::endl; } );

#ifndef NDEBUG
   std::clog << "GLFW DEBUG CONTEXT ENABLED" << std::endl;
   glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );
#endif
}

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
   _ASSERT_EXPR( !m_fRunning, L"Window is already running" );

   // glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   m_Window   = glfwCreateWindow( m_WindowData.Width, m_WindowData.Height, m_WindowData.Title.c_str(), nullptr, nullptr );
   m_fRunning = static_cast< bool >( m_Window );

   // Set OpenGL Version
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
   glfwMakeContextCurrent( m_Window );
   glfwSetWindowUserPointer( m_Window, this );
   glfwSwapInterval( m_WindowData.VSync );

   { // OpenGL Related
      // Failed to initialize OpenGL context
      assert( gladLoadGL() );

      // Depth Test
      glEnable( GL_DEPTH_TEST ); // learn more about what this does
      glDepthFunc( GL_LESS );

      // MSAA
      // glfwWindowHint(GLFW_SAMPLES, 4);
      // glEnable(GL_MULTISAMPLE);

      // Enable Face Culling and Winding Order
      glEnable( GL_CULL_FACE );
      glCullFace( GL_BACK );
      glFrontFace( GL_CCW );

      // Enable Alpha Blending
      glEnable( GL_BLEND );
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

      // Enable smooth shading
      glShadeModel( GL_SMOOTH ); // GL_FLAT or GL_SMOOTH (default)

      // Set clear color for the screen
      glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ); // Set screen background color (black)
   }

   // Set callbacks at the end
   SetCallbacks();

   // ImGui (this needs to be after setting the callbacks)
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui::GetIO().IniFilename = nullptr;
   ImGui::StyleColorsDark();
   ImGui_ImplGlfw_InitForOpenGL( m_Window, true );
   ImGui_ImplOpenGL3_Init( "#version 330" );
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
bool Window::FMinimized()
{
   return m_fMinimized;
}

bool Window::IsOpen()
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
   m_eventSubscriber.Subscribe< Events::WindowCloseEvent >( [ & ]( const Events::WindowCloseEvent& /*event*/ ) { m_fRunning = false; } );

   m_eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ & ]( const Events::WindowResizeEvent& event )
   {
      m_WindowData.Width  = event.GetWidth();
      m_WindowData.Height = event.GetHeight();
      glViewport( 0, 0, m_WindowData.Width, m_WindowData.Height );
   } );

   m_eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& event )
   {
      switch( event.GetKeyCode() )
      {
         case Events::KeyCode::Escape:
         {
            m_fRunning = false;
            break;
         }
         case Events::KeyCode::P:
         {
            static int polygonModeFlip = GL_FILL;
            polygonModeFlip            = ( polygonModeFlip == GL_FILL ) ? GL_LINE : GL_FILL;
            glPolygonMode( GL_FRONT_AND_BACK, polygonModeFlip );
            break;
         }
         case Events::KeyCode::F1:
         {
            static int fFlip = GLFW_CURSOR_NORMAL;
            fFlip            = ( fFlip == GLFW_CURSOR_NORMAL ) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
            glfwSetInputMode( m_Window, GLFW_CURSOR, fFlip );
            break;
         }

         default: break;
      }
   } );

   // OpenGL Error Callback (API RELATED, RELOCATE)
   glEnable( GL_DEBUG_OUTPUT );
   glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS ); // enable only on debug
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

   // GLFW Window Callbacks
   glfwSetWindowSizeCallback( m_Window, []( GLFWwindow* window, int width, int height ) { Events::Dispatch< Events::WindowResizeEvent >( width, height ); } );
   glfwSetWindowCloseCallback( m_Window, []( GLFWwindow* window ) { Events::Dispatch< Events::WindowCloseEvent >(); } );

   // GLFW Key Callbacks
   glfwSetKeyCallback( m_Window,
                       []( GLFWwindow* window, int key, int scancode, int action, int mods )
   {
      Events::KeyCode keyCode = static_cast< Events::KeyCode >( key );
      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::KeyPressedEvent >( keyCode, false /*fRepeated*/ ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::KeyReleasedEvent >( keyCode ); break;
         case GLFW_REPEAT:  Events::Dispatch< Events::KeyPressedEvent >( keyCode, true /*fRepeated*/ ); break;
         default:           throw std::runtime_error( "Unknown KeyEvent" );
      }
   } );

   glfwSetCharCallback( m_Window, []( GLFWwindow* window, unsigned int keycode ) {
      Events::Dispatch< Events::KeyTypedEvent >( static_cast< Events::KeyCode >( keycode ) );
   } );

   glfwSetMouseButtonCallback( m_Window,
                               []( GLFWwindow* window, int button, int action, int mods )
   {
      Events::MouseCode mouseCode = static_cast< Events::MouseCode >( button );
      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::MouseButtonPressedEvent >( mouseCode ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::MouseButtonReleasedEvent >( mouseCode ); break;
         default:           throw std::runtime_error( "Unknown MouseButtonEvent" );
      }
   } );

   glfwSetScrollCallback( m_Window, []( GLFWwindow* window, double xOffset, double yOffset ) {
      Events::Dispatch< Events::MouseScrolledEvent >( static_cast< float >( xOffset ), static_cast< float >( yOffset ) );
   } );

   glfwSetCursorPosCallback( m_Window, []( GLFWwindow* window, double xPos, double yPos ) {
      Events::Dispatch< Events::MouseMovedEvent >( static_cast< float >( xPos ), static_cast< float >( yPos ) );
   } );
}
