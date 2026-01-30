#pragma once

#include <Engine/Core/GameStateStack.h>

// Forward declarations
namespace UI
{
class UIContext;
}

namespace Game
{

// ----------------------------------------------------------------
// GameState - Base class for all game states
// ----------------------------------------------------------------
class GameState
{
public:
   virtual ~GameState() = default;

   // Lifecycle hooks
   virtual void OnEnter() = 0; // Called when state becomes active
   virtual void OnExit()  = 0; // Called when state is removed
   virtual void OnPause() {}   // Called when another state is pushed on top
   virtual void OnResume() {}  // Called when state above is popped

   // Frame updates
   virtual void Update( float dt )                = 0;
   virtual void FixedUpdate( float tickInterval ) = 0;
   virtual void Render()                          = 0;
   virtual void DrawUI( UI::UIContext& ui )       = 0;

protected:
   // Access shared context
   [[nodiscard]] StateContext&        Ctx() noexcept { return *m_ctx; }
   [[nodiscard]] const StateContext&  Ctx() const noexcept { return *m_ctx; }
   [[nodiscard]] Engine::GameContext& GameCtx() noexcept { return m_ctx->gameCtx; }

   // State operations
   template< typename T, typename... Args >
   void PushState( Args&&... args )
   {
      m_ctx->stateStack.Push< T >( std::forward< Args >( args )... );
   }

   template< typename T, typename... Args >
   void SwitchState( Args&&... args )
   {
      m_ctx->stateStack.Switch< T >( std::forward< Args >( args )... );
   }

   void PopState() { m_ctx->stateStack.Pop(); }

private:
   friend class ::GameStateStack;
   void SetContext( StateContext& ctx ) noexcept { m_ctx = &ctx; }

   StateContext* m_ctx { nullptr };
};

} // namespace Game
