#pragma once

// Project dependencies
#include <Engine/Events/Event.h>

// Forward Declarations
struct GLFWwindow;

struct WindowState
{
   // Identity & Config
   std::string title { "Title" };
   glm::uvec2  size { 1600, 900 };
   bool        fVSync { true };

   // Input State
   glm::vec2 mousePos { 0.0f };
   glm::vec2 mouseDelta { 0.0f };
   bool      fMouseCaptured { false };

   // Lifecycle
   bool fShouldClose { false };
   bool fMinimized { false };
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

   void Init();
   bool FProcessEvents();
   void Present();

   // WindowState
   const WindowState& GetWindowState() const { return m_state; }
   WindowState&       GetWindowState() { return m_state; }

   bool FMinimized() const noexcept { return m_state.fMinimized; }
   void SetVSync( bool fState );

   void OnFocusChanged( bool /*fFocused*/ ) noexcept {}

private:
   Window( std::string_view title ) noexcept; // private ctor for singleton
   ~Window();
   NO_COPY_MOVE( Window )

   GLFWwindow* m_window { nullptr };
   WindowState m_state;

   // Events
   Events::EventSubscriber m_eventSubscriber;
   void                    SetCallbacks();
};
