
#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Camera.h"
#include "../../Network/Network.h"

#include "GUIManager.h"

// Ordering of initialization is critical
void Application::Init()
{
   Window::Get().Init(); // Initialize the window
   GUIManager::Init();   // Initialize the GUI Manager
}

// Cleanup should occur in reverse order of initialization
void Application::Shutdown()
{
   GUIManager::Shutdown();
}

void Application::Run()
{
   Init(); // Initialize the application

   // TODO:
   // Write a RenderQueue, IGameObjects will be submitted to the RenderQueue
   //      After, the RenderQueue will be rendered
   // Write a logging library (log to file)
   //    Get rid of console window, pipe output to a imgui window

   // PlayerManager::GetPlayer() - returns the player
   //    only one player should theoretically exist at a time

   // https://github.com/htmlboss/OpenGL-Renderer

   World world;
   world.Setup();

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& e ) noexcept
   {
      if( e.GetKeyCode() == Input::R ) // Reload world
         world.Setup();
   } );

   Player player;
   Camera camera;

   // Attach debug UI to track player/camera
   GUIManager::Attach( std::make_shared< DebugGUI >( player, camera ) );

   // Attach Network GUI
   INetwork::RegisterGUI();

   float    time = 0;
   Timestep timestep( 20 /*tickrate*/ );
   Window&  window = Window::Get();
   while( window.IsOpen() )
   {
      {
         Events::ProcessQueuedEvents(); // Process queued events before the next frame
         //Network::ProcessQueuedEvents(); // Process network events before the next frame
      }

      // Game Ticks
      const float delta = timestep.Step();
      world.Tick( delta, camera ); // Update world
      player.Tick( world, delta ); // Tick Player, use m_World to check collisions

      // Game Updates
      player.Update( delta );  // Update Player
      camera.Update( player ); // Update Camera to match Player

      if( timestep.FTick() )
      {
         if( INetwork* pNetwork = INetwork::Get() )
            pNetwork->Poll( world, player ); // Poll network events
      }

      // Render only if not minimized
      if( !window.FMinimized() )
      {
         // Render
         world.Render( camera );

         // Render
         //RenderManager::Render();

         // Draw GUI
         GUIManager::Draw();
      }

      window.OnUpdate(); // Poll events and swap buffers
   }

   Shutdown(); // Cleanup the application
}

// NOTES
// https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
//      https://github.com/Hopson97/open-builder/issues/208#issuecomment-706747718
