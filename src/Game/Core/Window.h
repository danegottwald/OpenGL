#pragma once

#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Events/ApplicationEvent.h"

// Forward Declarations
struct GLFWwindow;

// Type Aliases
using EventCallbackFn = std::function<void( IEvent& )>;

struct WindowData
{
    std::string Title = "OpenGL";
    uint16_t Width = 1280;
    uint16_t Height = 720;
    bool VSync = true;
};

// --------------------------------------------------------------------
//      Window Class Singlton
// --------------------------------------------------------------------
class Window
{
public:
    // Singleton Related
    static Window& Get();
    static GLFWwindow* GetNativeWindow();
    Window( const Window& ) = delete;
    Window& operator=( const Window& ) = delete;
    Window( Window&& ) = delete;
    Window& operator=( Window&& ) = delete;

    ~Window();

    void Init();
    void OnUpdate() const;
    void Close();

    glm::vec2 GetMousePosition() const;

    // Queries
    bool FMinimized();
    bool IsOpen();

    // WindowData
    WindowData& GetWindowData();
    void SetVSync( bool state );

    // Events
    bool OnEvent( EventDispatcher& e );
    void SetEventCallback( const EventCallbackFn& callback );
    EventCallbackFn& EventCallback();

private:
    // Disallow instantiation outside of class
    Window( const WindowData& config );

    GLFWwindow* m_Window = nullptr;
    WindowData m_WindowData;

    bool m_fMinimized = false;
    bool m_fRunning = false;

    // Events
    EventCallbackFn m_eventCallback;
    bool EventResize( WindowResizeEvent& e );
    bool EventClose( WindowCloseEvent& e );
    void SetCallbacks() const;
};
