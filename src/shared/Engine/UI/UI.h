#pragma once

#include <imgui.h>

// Forward Declarations
struct GLFWwindow;
class Window;

namespace Entity
{
class Registry;
using Entity = uint64_t;
}

namespace Time
{
class FixedTimeStep;
}

namespace UI
{

// ----------------------------------------------------------------
// IDrawable - Interface for UI elements that can be drawn each frame
// ----------------------------------------------------------------
struct IDrawable
{
   virtual ~IDrawable() = default;
   virtual void Draw()  = 0;
};


// ----------------------------------------------------------------
// UIContext - Manages the UI system and provides access to display info
// ----------------------------------------------------------------
class UIContext
{
public:
   explicit UIContext( const Window& window ) noexcept :
      m_window( window )
   {}
   ~UIContext();

   void Init( GLFWwindow* pGlfwWindowHandle );
   void Shutdown();

   void BeginFrame();
   void Register( std::shared_ptr< IDrawable > element );
   void EndFrame();

   // Display info for UI layout
   [[nodiscard]] const Window& GetWindow() const noexcept { return m_window; }
   [[nodiscard]] glm::uvec2    GetWindowSize() const noexcept;

private:
   const Window& m_window;

   bool m_fInitialized { false };
   bool m_fFrameActive { false };

   static constexpr size_t                     kInitialUIElementCapacity = 128;
   std::vector< std::shared_ptr< IDrawable > > m_uiElements;
};

} // namespace UI

std::shared_ptr< UI::IDrawable > CreateDebugUI( const Entity::Registry&    registry,
                                                Entity::Entity             player,
                                                Entity::Entity             camera,
                                                const Time::FixedTimeStep& timestep );
