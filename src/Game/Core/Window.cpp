#include "Window.h"

Window::Window( const WindowData& winData ) : m_WindowData( winData )
{
    // Initialize GLFW
    uint32_t success = glfwInit();
    _ASSERT_EXPR( success == GLFW_TRUE, L"Failed to initialize GLFW" );
    glfwSetErrorCallback( []( int code, const char* message )
        {
            std::cout << "[GLFW Error] (" << code << "): " << message << std::endl;
        } );

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
    static Window instance = Window( WindowData{ "OpenGL" } );
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
    m_Window = glfwCreateWindow( m_WindowData.Width, m_WindowData.Height, m_WindowData.Title.c_str(), nullptr, nullptr );
    m_fRunning = ( bool )m_Window;

    glfwMakeContextCurrent( m_Window );
    glfwSetWindowUserPointer( m_Window, this );

    glfwSwapInterval( m_WindowData.VSync );

    // API related
    // Load OpenGL Functions, returns OpenGL Version

    //uint32_t success = gladLoadGL( ( GLADloadfunc )glfwGetProcAddress );
    uint32_t success = gladLoadGL();
    _ASSERT_EXPR( success, L"Failed to initialize OpenGL context" );

    // MSAA
    // glfwWindowHint(GLFW_SAMPLES, 4);
    // glEnable(GL_MULTISAMPLE);

    // Enable Face Culling and Winding Order
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CCW);
    // End API related

    // glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if( m_eventCallback )
        SetCallbacks();
}

void Window::OnUpdate() const
{
    //Renderer::Get().Draw();

    // Poll for and process events
    glfwPollEvents();

    // Swap front and back buffers
    glfwSwapBuffers( m_Window );
}

void Window::Close()
{
    m_fRunning = false;
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

// --------------------------------------------------------------------
//      Window Events
// --------------------------------------------------------------------
void Window::SetEventCallback( const EventCallbackFn& callback )
{
    m_eventCallback = callback;
}

EventCallbackFn& Window::EventCallback()
{
    return m_eventCallback;
}

bool Window::OnEvent( EventDispatcher& dispatcher )
{
    IEvent& event = dispatcher.GetEvent();
    switch( event.GetEventType() )
    {
        case EventType::WindowClose:
            return dispatcher.Dispatch<WindowCloseEvent>(
                std::bind( &Window::EventClose, this, std::placeholders::_1 ) );
        case EventType::WindowResize:
            return dispatcher.Dispatch<WindowResizeEvent>(
                std::bind( &Window::EventResize, this, std::placeholders::_1 ) );
        case EventType::WindowFocus:
            // Possibly move to Game? make focus grab mouse?
            return false;
        case EventType::WindowLostFocus:
            return false;
        case EventType::WindowMoved:
            return false;
        case EventType::KeyPressed:
        {
            switch( static_cast< KeyEvent& >( event ).GetKeyCode() )
            {
                case KeyCode::P:
                {
                    static int polygonModeFlip = GL_FILL;
                    polygonModeFlip = ( polygonModeFlip == GL_FILL ) ? GL_LINE : GL_FILL;
                    glPolygonMode( GL_FRONT_AND_BACK, polygonModeFlip );
                    return true;
                }
                case KeyCode::Escape:
                    Close();
                    return true;
                case KeyCode::F1:
                    static int fFlip = GLFW_CURSOR_NORMAL;
                    fFlip = fFlip == GLFW_CURSOR_NORMAL ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
                    glfwSetInputMode( m_Window, GLFW_CURSOR, fFlip );
                    return true;
            }
            return false;
        }
        default:
            return false;
    }
}


bool Window::EventResize( WindowResizeEvent& e )
{
    m_WindowData.Width = e.GetWidth();
    m_WindowData.Height = e.GetHeight();

    glViewport( 0, 0, m_WindowData.Width, m_WindowData.Height );

    return ( m_fMinimized = ( m_WindowData.Width == 0 || m_WindowData.Height == 0 ) );
}


bool Window::EventClose( WindowCloseEvent& e )
{
    return !( m_fRunning = false );
}

/*
    How the callback system works
        In Application Constructor
            m_Window.SetEventCallback(
                std::bind(&Application::OnEvent, this, std::placeholders::_1));

        In Application::OnEvent, set a dispatcher for each event
            dispatcher.Dispatch<WindowCloseEvent>(
                std::bind(&Window::EventClose, &m_Window,
                    std::placeholders::_1));

            dispatcher.Dispatch<WindowResizeEvent>(
                std::bind(&Window::EventResize, &m_Window,
                    std::placeholders::_1));

        Every event from the callback's below will send the event to the
        Window::EventCallback() which is the bound function pointer from
        Application's constructor

        To keep the callbacks generic, new function pointers can be bound
        to the dispatcher if you want to change functionality of the
        specific event.

        Every callback must return a boolean meaning if the event has
        been handled or not (true: handled, false: not handled)
*/
void Window::SetCallbacks() const
{
    // OpenGL Error Callback (API RELATED, RELOCATE)
    glEnable( GL_DEBUG_OUTPUT );
    glDebugMessageCallback(
        []( GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const char* message, const void* userParam )
        {
            // if (type != 33361)
            //{
            std::cout << "[ERROR](OpenGL) [" << id << "]: " << message << std::endl;
            //}
        },
        nullptr );

    // GLFW Window Callbacks
    glfwSetWindowSizeCallback( m_Window,
        []( GLFWwindow* window, int width, int height )
        {
            Window::Get().EventCallback()( WindowResizeEvent( width, height ) );
        } );

    glfwSetWindowCloseCallback( m_Window,
        []( GLFWwindow* window )
        {
            Window::Get().EventCallback()( WindowCloseEvent() );
        } );

    // GLFW Key Callbacks
    glfwSetKeyCallback( m_Window,
        []( GLFWwindow* window, int key, int scancode, int action, int mods )
        {
            KeyEvent* event = nullptr;
            KeyCode keyCode = static_cast< KeyCode >( key );
            switch( action )
            {
                case GLFW_PRESS:
                    event = &KeyPressedEvent( keyCode, false /*fRepeated*/ );
                    break;
                case GLFW_RELEASE:
                    event = &KeyReleasedEvent( keyCode );
                    break;
                case GLFW_REPEAT:
                    event = &KeyPressedEvent( keyCode, true /*fRepeated*/ );
                    break;
                default:
                    throw std::runtime_error( "Unknown KeyEvent" );
            }
            Window::Get().EventCallback()( *event );
        } );

    glfwSetCharCallback( m_Window,
        []( GLFWwindow* window, unsigned int keycode )
        {
            Window::Get().EventCallback()( KeyTypedEvent( static_cast< KeyCode >( keycode ) ) );
        } );

    // GLFW Mouse Callbacks
    glfwSetMouseButtonCallback( m_Window,
        []( GLFWwindow* window, int button, int action, int mods )
        {
            MouseButtonEvent* event = nullptr;
            MouseCode mouseCode = static_cast< MouseCode >( button );
            switch( action )
            {
                case GLFW_PRESS:
                    event = &MouseButtonPressedEvent( mouseCode );
                    break;
                case GLFW_RELEASE:
                    event = &MouseButtonReleasedEvent( mouseCode );
                    break;
                default:
                    throw std::runtime_error( "Unknown MouseButtonEvent" );
            }
            Window::Get().EventCallback()( *event );
        } );

    glfwSetScrollCallback( m_Window,
        []( GLFWwindow* window, double xOffset, double yOffset )
        {
            Window::Get().EventCallback()( MouseScrolledEvent( xOffset, yOffset ) );
        } );

    glfwSetCursorPosCallback( m_Window, []( GLFWwindow* window, double xPos, double yPos )
        {
            Window::Get().EventCallback()( MouseMovedEvent( xPos, yPos ) );
        } );
}
