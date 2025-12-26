#pragma once

namespace UI
{


/**
 * @brief Interface for drawable UI elements.
 * This interface defines a contract for UI elements that can be drawn.
 */
struct IDrawable
{
   virtual ~IDrawable() = default;
   virtual void Draw()  = 0;
};


/**
 * @brief UI Buffer for managing drawable UI elements.
 * This class is responsible for storing and rendering UI elements once per frame.
 * 
 * Register UI elements using `Register()` and call `Draw()` each frame to render them.
 */
class UIBuffer
{
public:
   static void Init();
   static void Shutdown();

   static void Register( std::shared_ptr< IDrawable > element );
   static void Draw();

private:
   static inline bool                                        s_fInitialized            = false;
   static inline constexpr size_t                            kInitialUIElementCapacity = 100;
   static inline std::vector< std::shared_ptr< IDrawable > > s_uiElements;
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
