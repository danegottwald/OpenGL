#include "Application.h"

// Local dependencies
#include "Time.h"
#include "Window.h"

// Project dependencies
#include <Engine/Core/GameContext.h>
#include <Engine/ECS/Registry.h>
#include <Engine/Events/Event.h>
#include <Game/Game.h>
#include <Game/InGameState.h>
#include <Game/MainMenuState.h>


void Application::Run()
{
   Window& window = Window::Get();
   window.Init();

   bool fStartGameRequested = false;
   auto makeMenu            = [ & ]() -> std::unique_ptr< Game::Game > { return std::make_unique< Game::MainMenuState >( fStartGameRequested ); };
   auto makeInGame          = [ & ]() -> std::unique_ptr< Game::Game > { return std::make_unique< Game::InGameState >(); };

   Time::FixedTimeStep timestep( 20 /*tickrate*/ );
   m_pRegistry = std::make_unique< Entity::Registry >();
   Engine::GameContext ctx( window, timestep, *m_pRegistry );

   m_state = APP_STATE_MENU;
   m_pGame = makeMenu();
   m_pGame->OnEnter( ctx );

   while( window.FBeginFrame() )
   {
      const float dt = std::clamp( timestep.Step(), 0.0f, 0.05f /*50ms*/ );

      Events::ProcessQueuedEvents();

      if( m_pGame )
      {
         m_pGame->Update( ctx, dt );

         if( timestep.FShouldTick() )
            m_pGame->FixedUpdate( ctx, dt );

         m_pGame->Render( ctx );
         m_pGame->DrawUI( ctx );
      }

      // Handle state transitions
      switch( m_state )
      {
         case APP_STATE_MENU:
         {
            if( !fStartGameRequested )
               break;

            fStartGameRequested = false;
            m_state             = APP_STATE_LOADING;
            break;
         }

         case APP_STATE_LOADING:
         {
            if( m_pGame )
               m_pGame->OnExit( ctx );

            m_pGame = makeInGame();
            m_state = APP_STATE_INGAME;
            m_pGame->OnEnter( ctx );
            break;
         }

         default: break;
      }

      window.EndFrame();
   }

   if( m_pGame )
      m_pGame->OnExit( ctx );
}
