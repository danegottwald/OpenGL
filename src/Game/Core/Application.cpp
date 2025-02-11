
#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Camera.h"

#include "GUIManager.h"

// Ordering of initialization is critical
void Application::Init()
{
   Window::Get().Init(); // Initialize the window
   GUIManager::Init();   // Initialize the GUI Manager
}

// Cleanup should occur in reverse order of initialization
void Application::Reset()
{
   GUIManager::Reset();
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

   // Attach Debug GUI
   GUIManager::Attach( std::make_shared< DebugGUI >() );

   World world;
   world.Setup();
   Player player;
   Camera camera;

   Timestep timestep;
   Window&  window = Window::Get();
   while( window.IsOpen() )
   {
      timestep.Step(); // Compute delta

      // Game Ticks
      float delta = timestep.GetDelta();
      world.Tick( delta );         // Update world
      player.Tick( world, delta ); // Tick Player, use m_World to check collisions

      // Game Updates
      player.Update( delta );  // Update Player
      camera.Update( player ); // Update Camera to match Player

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

      // RENDER() from MAIN LOOP
      // glEnable(GL_DEPTH_TEST);
      //
      //// World
      // m_worldRenderTarget.bind();
      // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      // m_game.render();
      //
      //// GUI
      // m_guiRenderTarget.bind();
      // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      // m_gui.render(m_guiRenderer);
      //
      //// Buffer to window
      // gl::unbindFramebuffers(ClientConfig::get().windowWidth,
      //                         ClientConfig::get().windowHeight);
      // glDisable(GL_DEPTH_TEST);
      // glClear(GL_COLOR_BUFFER_BIT);
      // auto drawable = m_screenBuffer.getDrawable();
      // drawable.bind();
      // m_screenShader.bind();
      //
      // m_worldRenderTarget.bindTexture();
      // drawable.draw();
      //
      // glEnable(GL_BLEND);
      //
      // m_guiRenderTarget.bindTexture();
      // drawable.draw();
      //
      // mp_window->display();
      // glDisable(GL_BLEND);
   }

   Reset(); // Cleanup the application
}

// NOTES
// https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
//      https://github.com/Hopson97/open-builder/issues/208#issuecomment-706747718
