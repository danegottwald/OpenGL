#include "Window.h"

// Local dependencies
#include "RenderContext.h"

// Project dependencies
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Input/Input.h>


// --------------------------------------------------------------------
// Window
// --------------------------------------------------------------------
Window::Window( std::string_view title ) noexcept :
   m_state( WindowState { .title = title.data() } )
{}


Window::~Window()
{
   m_ui.Shutdown(); // UI context depends on GLFW window and GL context, so shut it down first
   m_pRenderContext.reset();

   glfwDestroyWindow( m_pWindow );
   glfwTerminate();
}


Window& Window::Get()
{
   static Window instance = Window( "OpenGL" );
   return instance;
}


void Window::Init()
{
   if( !glfwInit() )
      throw std::runtime_error( "Failed to initialize GLFW" );

   glfwSetErrorCallback( []( int code, const char* message ) { std::println( std::cerr, "[GLFW Error] ({}) {}", code, message ); } );

#ifndef NDEBUG
   std::clog << "GLFW DEBUG CONTEXT ENABLED" << std::endl;
   glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );
#endif

   // OpenGL window hints (API-specific, but required before window creation)
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   glfwWindowHint( GLFW_DEPTH_BITS, 24 );
   glfwWindowHint( GLFW_SAMPLES, 4 );

   m_pWindow = glfwCreateWindow( m_state.size.x, m_state.size.y, m_state.title.c_str(), nullptr, nullptr );
   if( !m_pWindow )
      throw std::runtime_error( "Failed to create GLFW window" );

   glfwSetWindowUserPointer( m_pWindow, this );

   SetCallbacks();

   // RenderContext owns GLAD + GL state + swap/viewport/debug
   m_pRenderContext = std::make_unique< RenderContext >( *m_pWindow );
   m_pRenderContext->Init( m_state.fVSync );

   m_ui.Init( m_pWindow );
}


bool Window::FCheckForShutdown()
{
   if( glfwWindowShouldClose( m_pWindow ) )
      m_state.fShouldClose = true;

   if( m_state.fShouldClose )
      glfwSetWindowShouldClose( m_pWindow, GLFW_TRUE );

   return m_state.fShouldClose;
}


void Window::UpdateMouseState()
{
   double x, y;
   glfwGetCursorPos( m_pWindow, &x, &y );
   glm::vec2 currentPos = { ( float )x, ( float )y };

   const bool fCaptured   = ( glfwGetInputMode( m_pWindow, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED );
   m_state.mouseDelta     = ( fCaptured && !m_state.fMouseCaptured ) ? glm::vec2( 0.0f ) : currentPos - m_state.mousePos;
   m_state.mousePos       = currentPos;
   m_state.fMouseCaptured = fCaptured;
}


bool Window::FBeginFrame()
{
   if( FCheckForShutdown() )
      return false;

   glfwPollEvents();

   // Update states after event processing
   UpdateMouseState();

   // Begin UI frame early; game code can submit UI throughout the frame.
   m_ui.BeginFrame();

   return true;
}


void Window::EndFrame()
{
   // UI renders just before present.
   m_ui.EndFrame();

   if( m_pRenderContext )
      m_pRenderContext->Present();
}


void Window::SetVSync( bool fState )
{
   if( fState == m_state.fVSync )
      return;

   m_state.fVSync = fState;
   if( m_pRenderContext )
      m_pRenderContext->SetVSync( fState );
}


void Window::SetCallbacks()
{
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
            glfwSetInputMode( m_pWindow, GLFW_CURSOR, fFlip );

            int width, height;
            glfwGetWindowSize( m_pWindow, &width, &height );
            glfwSetCursorPos( m_pWindow, width * 0.5f, height * 0.5f );
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
               glfwSetWindowAttrib( m_pWindow, GLFW_AUTO_ICONIFY, GLFW_FALSE );
               glfwSetWindowMonitor( m_pWindow, primary, 0, 0, mode->width, mode->height, mode->refreshRate );
            }
            else
            {
               const float paddingPercent = 0.1f;
               int         padX           = static_cast< int >( mode->width * paddingPercent );
               int         padY           = static_cast< int >( mode->height * paddingPercent );
               int         windowedWidth = mode->width - padX, windowedHeight = mode->height - padY;
               int         windowedX = padX / 2, windowedY = padY / 2;
               glfwSetWindowMonitor( m_pWindow, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0 );
            }
            break;
         }

         default: break;
      }
   } );

   glfwSetWindowCloseCallback( m_pWindow,
                               []( GLFWwindow* window )
   {
      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
         pWindow->GetWindowState().fShouldClose = true;

      Events::Dispatch< Events::WindowCloseEvent >();
   } );

   glfwSetWindowIconifyCallback( m_pWindow,
                                 []( GLFWwindow* window, int iconified )
   {
      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
         pWindow->GetWindowState().fMinimized = ( iconified == GLFW_TRUE );
   } );

   glfwSetFramebufferSizeCallback( m_pWindow,
                                   []( GLFWwindow* window, int width, int height )
   {
      if( width == 0 || height == 0 )
         return;

      if( Window* pWindow = static_cast< Window* >( glfwGetWindowUserPointer( window ) ) )
      {
         pWindow->GetWindowState().size = { static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) };

         // Inform the render context of the size change prior to dispatching event
         if( pWindow->m_pRenderContext )
            pWindow->m_pRenderContext->UpdateViewport( 0, 0, width, height );
      }

      Events::Dispatch< Events::WindowResizeEvent >( width, height );
   } );

   glfwSetWindowFocusCallback( m_pWindow,
                               []( GLFWwindow* window, int focused )
   {
      if( static_cast< bool >( focused ) )
         Events::Dispatch< Events::WindowFocusEvent >();
      else
         Events::Dispatch< Events::WindowLostFocusEvent >();
   } );

   glfwSetKeyCallback( m_pWindow,
                       []( GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/ )
   {
      if( ImGui::GetIO().WantTextInput )
         return;

      Input::KeyCode keyCode = static_cast< Input::KeyCode >( key );
      if( action != GLFW_REPEAT )
         Input::SetKeyPressed( keyCode, static_cast< bool >( action ) );

      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::KeyPressedEvent >( keyCode, false ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::KeyReleasedEvent >( keyCode ); break;
         case GLFW_REPEAT:  Events::Dispatch< Events::KeyPressedEvent >( keyCode, true ); break;
         default:           throw std::runtime_error( "Unknown KeyEvent" );
      }
   } );

   glfwSetCharCallback( m_pWindow,
                        []( GLFWwindow* /*window*/, unsigned int keycode )
   { Events::Dispatch< Events::KeyTypedEvent >( static_cast< Input::KeyCode >( keycode ) ); } );

   glfwSetMouseButtonCallback( m_pWindow,
                               []( GLFWwindow* /*window*/, int button, int action, int /*mods*/ )
   {
      if( ImGui::GetIO().WantCaptureMouse )
         return;

      Input::MouseCode mouseCode = static_cast< Input::MouseCode >( button );
      Input::SetMouseButtonPressed( mouseCode, action != GLFW_RELEASE );

      switch( action )
      {
         case GLFW_PRESS:   Events::Dispatch< Events::MouseButtonPressedEvent >( mouseCode ); break;
         case GLFW_RELEASE: Events::Dispatch< Events::MouseButtonReleasedEvent >( mouseCode ); break;
         default:           throw std::runtime_error( "Unknown MouseButtonEvent" );
      }
   } );

   glfwSetScrollCallback( m_pWindow,
                          []( GLFWwindow* /*window*/, double xOffset, double yOffset )
   {
      if( ImGui::GetIO().WantCaptureMouse )
         return;

      Events::Dispatch< Events::MouseScrolledEvent >( static_cast< float >( xOffset ), static_cast< float >( yOffset ) );
   } );
}
