#include "Application.h"

// Local dependencies
#include "Time.h"
#include "GameStateStack.h"

// Project dependencies
#include <Engine/Platform/Window.h>
#include <Engine/Core/GameContext.h>
#include <Engine/ECS/Registry.h>
#include <Engine/Events/Event.h>
#include <Game/GameState.h>
#include <Game/MainMenuState.h>


// ----------------------------------------------------------------
// Application
// ----------------------------------------------------------------
Application::Application() noexcept  = default;
Application::~Application() noexcept = default;


void Application::Run()
{
   Window& window = Window::Get();
   window.Init();

   Time::FixedTimeStep timestep( 20 /*tickrate*/ );
   m_pRegistry = std::make_unique< Entity::Registry >();
   Engine::GameContext gameCtx( window, timestep, *m_pRegistry );

   // Create state stack and start with MainMenu
   GameStateStack stateStack( gameCtx );
   stateStack.Push< Game::MainMenuState >();
   //stateStack.ProcessPendingChanges();

   while( window.FBeginFrame() )
   {
      const float dt = timestep.Advance( 0.25f /*maxDelta*/ );

      Events::ProcessQueuedEvents();

      stateStack.Update( dt );
      while( timestep.FTryAdvanceTick() )
         stateStack.FixedUpdate( timestep.GetTickInterval() );

      stateStack.Render();
      stateStack.DrawUI( window.GetUIC() );

      // Process any pending state changes at frame boundary
      if( stateStack.FHasPendingChanges() )
         stateStack.ProcessPendingChanges();

      // Exit if all states have been popped
      if( stateStack.FEmpty() )
         break;

      window.EndFrame();
   }
}
