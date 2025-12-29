#pragma once

// Forward Declarations
struct GLFWwindow;

namespace UI
{

struct IDrawable
{
   virtual ~IDrawable() = default;
   virtual void Draw()  = 0;
};

// Immediate-mode style UI context.
// - Submit UI commands (as IDrawable objects) throughout the frame.
// - Call `BeginFrame()` once, then `EndFrame()` once, per frame.
// - Submissions are cleared automatically each frame.
//
// The UI backend is initialized against a specific native window.

class UIContext
{
public:
   UIContext() = default;
   ~UIContext();

   void Init( GLFWwindow* pGlfwWindowHandle );
   void Shutdown();

   void BeginFrame();
   void Register( std::shared_ptr< IDrawable > element );
   void EndFrame();

   bool FInitialized() const noexcept { return m_fInitialized; }

private:
   NO_COPY_MOVE( UIContext )

   bool m_fInitialized { false };
   bool m_fFrameActive { false };

   static inline constexpr size_t              kInitialUIElementCapacity = 128;
   std::vector< std::shared_ptr< IDrawable > > m_uiElements;
};

} // namespace UI

namespace Entity
{
using Entity = uint64_t;
class Registry;
}

namespace Time
{
class FixedTimeStep;
}

std::shared_ptr< UI::IDrawable > CreateDebugUI( const Entity::Registry&    registry,
                                                Entity::Entity             player,
                                                Entity::Entity             camera,
                                                const Time::FixedTimeStep& timestep );
