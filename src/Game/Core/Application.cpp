
#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Camera.h"

/*static*/ Application& Application::Get() noexcept
{
   static Application app;
   return app;
}


void Application::Run()
{
   Window& window = Window::Get();
   window.Init();

   // TODO:
   // Write a RenderQueue, IGameObjects will be submitted to the RenderQueue
   //      After, the RenderQueue will be rendered

   World world;
   world.Setup();
   Player player;
   Camera camera;

   Timestep timestep;
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

         // Start ImGui Frame (only render when not minimized)
         {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            { // ImGui Top Left
               ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
               ImGui::Begin( "Performance",
                             nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
               ImGui::Text( "FPS: %.1f (%.1fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate );
               static bool fVSync = Window::Get().GetWindowData().VSync;
               if( ImGui::Checkbox( "VSync", &fVSync ) )
                  window.SetVSync( fVSync );
               ImGui::End();
            }

            { // ImGui Top Right
               ImGui::SetNextWindowPos( ImVec2( Window::Get().GetWindowData().Width - 10, 10 ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
               ImGui::Begin( "Key Bindings",
                             nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
               if( ImGui::BeginTable( "KeyBindingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
               {
                  // Set up column headers
                  ImGui::TableSetupColumn( "Key", ImGuiTableColumnFlags_WidthFixed, 100.0f );
                  ImGui::TableSetupColumn( "Action", ImGuiTableColumnFlags_WidthStretch );
                  ImGui::TableHeadersRow();
                  auto AddKeyBinding = []( const char* key, const char* action )
                  {
                     ImGui::TableNextRow();
                     ImGui::TableSetColumnIndex( 0 );
                     ImGui::TextUnformatted( key );
                     ImGui::TableSetColumnIndex( 1 );
                     ImGui::TextUnformatted( action );
                  };

                  AddKeyBinding( "Escape", "Quit" );
                  AddKeyBinding( "F1", "Toggle Mouse" );
                  AddKeyBinding( "P", "Toggle Wireframe" );
                  ImGui::EndTable();
               }
               ImGui::End();
            }

            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
         }
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

   { // cleanup ImGui
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
   }
}


// NOTES
// https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
//      https://github.com/Hopson97/open-builder/issues/208#issuecomment-706747718
