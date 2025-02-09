#pragma once

#include "../../Events/Event.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Events/ApplicationEvent.h"

// Forward Declarations
struct GLFWwindow;

struct WindowData
{
   std::string Title  = "OpenGL";
   uint16_t    Width  = 1280;
   uint16_t    Height = 720;
   bool        VSync  = true;
};

// --------------------------------------------------------------------
//      Window Class Singlton
// --------------------------------------------------------------------
class Window
{
public:
   // Singleton Related
   static Window&     Get();
   static GLFWwindow* GetNativeWindow();
   Window( const Window& )            = delete;
   Window& operator=( const Window& ) = delete;
   Window( Window&& )                 = delete;
   Window& operator=( Window&& )      = delete;

   ~Window();

   void Init();
   void OnUpdate() const;

   glm::vec2 GetMousePosition() const;

   // Queries
   bool FMinimized() const noexcept;
   bool IsOpen() const noexcept;

   // WindowData
   WindowData& GetWindowData();
   void        SetVSync( bool state );

private:
   // Disallow instantiation outside of class
   Window( const WindowData& config );

   GLFWwindow* m_Window = nullptr;
   WindowData  m_WindowData;

   bool m_fMinimized = false;
   bool m_fRunning   = false;

   // Events
   Events::EventSubscriber m_eventSubscriber;
   void                    SetCallbacks();
};
