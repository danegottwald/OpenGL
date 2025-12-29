#pragma once

// Project dependencies
#include <Engine/Core/UI.h>
#include <Engine/Events/Event.h>

// Forward Declarations
struct GLFWwindow;
class RenderContext;

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
   static Window& Get();

   void Init();
   bool FBeginFrame();
   void EndFrame();

   // UI
   UI::UIContext&       GetUIC() noexcept { return m_ui; }
   const UI::UIContext& GetUIC() const noexcept { return m_ui; }

   // WindowState
   const WindowState& GetWindowState() const { return m_state; }
   WindowState&       GetWindowState() { return m_state; }

   bool FMinimized() const noexcept { return m_state.fMinimized; }
   void SetVSync( bool fState );

private:
   Window( std::string_view title ) noexcept; // private ctor for singleton
   ~Window();
   NO_COPY_MOVE( Window )

   bool FCheckForShutdown(); // returns true if should close
   void UpdateMouseState();

   GLFWwindow* m_pWindow { nullptr };
   WindowState m_state;

   UI::UIContext                    m_ui;
   std::unique_ptr< RenderContext > m_pRenderContext;

   // Events
   Events::EventSubscriber m_eventSubscriber;
   void                    SetCallbacks();
};
