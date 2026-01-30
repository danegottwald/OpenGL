#include "GameStateStack.h"

#include <Engine/Core/GameContext.h>
#include <Game/GameState.h>


// ----------------------------------------------------------------
// GameStateStack
// ----------------------------------------------------------------
GameStateStack::GameStateStack( Engine::GameContext& gameCtx ) :
   m_ctx( *this, gameCtx )
{}


GameStateStack::~GameStateStack()
{
   // Exit all states in reverse order
   while( !m_stack.empty() )
   {
      m_stack.back()->OnExit();
      m_stack.pop_back();
   }
}


void GameStateStack::Pop()
{
   m_pendingChanges.push_back( { PendingChange::Action::Pop, nullptr } );
}


void GameStateStack::Clear()
{
   m_pendingChanges.push_back( { PendingChange::Action::Clear, nullptr } );
}


void GameStateStack::ProcessPendingChanges()
{
   for( PendingChange& change : m_pendingChanges )
   {
      switch( change.action )
      {
         case PendingChange::Action::Push: PushState( std::move( change.state ) ); break;

         case PendingChange::Action::Pop:
            if( !m_stack.empty() )
            {
               m_stack.back()->OnExit();
               m_stack.pop_back();

               // Resume the state below if it exists
               if( !m_stack.empty() )
                  m_stack.back()->OnResume();
            }
            break;

         case PendingChange::Action::Clear:
            while( !m_stack.empty() )
            {
               m_stack.back()->OnExit();
               m_stack.pop_back();
            }
            break;
      }
   }

   m_pendingChanges.clear();
}


void GameStateStack::PushState( std::unique_ptr< Game::GameState > state )
{
   // Pause the current top state
   if( !m_stack.empty() )
      m_stack.back()->OnPause();

   // Initialize and push the new state
   state->SetContext( m_ctx );
   m_stack.push_back( std::move( state ) );
   m_stack.back()->OnEnter();
}


void GameStateStack::Update( float dt )
{
   if( !m_stack.empty() )
      m_stack.back()->Update( dt );
}


void GameStateStack::FixedUpdate( float tickInterval )
{
   if( !m_stack.empty() )
      m_stack.back()->FixedUpdate( tickInterval );
}


void GameStateStack::Render()
{
   if( !m_stack.empty() )
      m_stack.back()->Render();
}


void GameStateStack::DrawUI( UI::UIContext& ui )
{
   if( !m_stack.empty() )
      m_stack.back()->DrawUI( ui );
}


Game::GameState* GameStateStack::GetCurrentState() const noexcept
{
   return !m_stack.empty() ? m_stack.back().get() : nullptr;
}
