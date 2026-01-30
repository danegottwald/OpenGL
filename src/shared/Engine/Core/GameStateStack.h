#pragma once

// Forward declarations
namespace Engine
{
class GameContext;
}

namespace Game
{
class GameState;
}

namespace UI
{
class UIContext;
}


// ----------------------------------------------------------------
// StateContext - Shared context available to all states
// ----------------------------------------------------------------
struct StateContext
{
   class GameStateStack& stateStack;
   Engine::GameContext&  gameCtx;
};


// ----------------------------------------------------------------
// GameStateStack - State stack with pending actions
// ----------------------------------------------------------------
class GameStateStack
{
public:
   explicit GameStateStack( Engine::GameContext& gameCtx );
   ~GameStateStack();
   NO_COPY_MOVE( GameStateStack )

   // State manipulation (queued, processed at safe points)
   template< typename T, typename... Args >
   void Push( Args&&... args )
   {
      static_assert( std::is_base_of_v< Game::GameState, T >, "T must derive from Game::GameState" );
      m_pendingChanges.push_back( { PendingChange::Action::Push, std::make_unique< T >( std::forward< Args >( args )... ) } );
   }

   template< typename T, typename... Args >
   void Switch( Args&&... args ) // Pop + Push atomically
   {
      static_assert( std::is_base_of_v< Game::GameState, T >, "T must derive from Game::GameState" );
      m_pendingChanges.push_back( { PendingChange::Action::Pop, nullptr } );
      m_pendingChanges.push_back( { PendingChange::Action::Push, std::make_unique< T >( std::forward< Args >( args )... ) } );
   }

   void Pop();
   void Clear();

   // Process queued state changes - call at frame boundaries
   void ProcessPendingChanges();

   // Update methods - operate on top state only
   void Update( float dt );
   void FixedUpdate( float tickInterval );
   void Render();
   void DrawUI( UI::UIContext& ui );

   [[nodiscard]] bool                FEmpty() const noexcept { return m_stack.empty(); }
   [[nodiscard]] bool                FHasPendingChanges() const noexcept { return !m_pendingChanges.empty(); }
   [[nodiscard]] Game::GameState*    GetCurrentState() const noexcept;
   [[nodiscard]] const StateContext& GetContext() const noexcept { return m_ctx; }

private:
   void PushState( std::unique_ptr< Game::GameState > state );

   struct PendingChange
   {
      enum class Action
      {
         Push,
         Pop,
         Clear
      };

      Action                             action;
      std::unique_ptr< Game::GameState > state; // Only valid for Push
   };

   StateContext                                      m_ctx;
   std::vector< std::unique_ptr< Game::GameState > > m_stack;
   std::vector< PendingChange >                      m_pendingChanges;
};
