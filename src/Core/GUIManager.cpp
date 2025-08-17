#include "GUIManager.h"

// Local dependencies
#include "Window.h"

// Project dependencies
#include <Core/Timestep.h>
#include <Entity/Registry.h>
#include <Entity/Components/Transform.h>
#include <Entity/Components/Velocity.h>

// ========================================================================
//      GUIManager
// ========================================================================
void GUIManager::Init()
{
   assert( m_fInitialized == false && L"GUIManager is already initialized" );

   GLFWwindow* nativeWindow = Window::GetNativeWindow();
   assert( nativeWindow && L"Ensure window is initialized before GUIManager" );

   IMGUI_CHECKVERSION();                               // Ensure correct ImGui version
   ImGui::CreateContext();                             // Create a new ImGui context
   ImGui::GetIO().IniFilename = nullptr;               // Disable saving ImGui settings to a file
   ImGui::StyleColorsDark();                           // Use dark theme for ImGui
   ImGui_ImplGlfw_InitForOpenGL( nativeWindow, true ); // Initialize ImGui with GLFW support
   if( !ImGui_ImplOpenGL3_Init( "#version 330" ) )
      throw std::runtime_error( "Failed to initialize ImGui OpenGL backend" );

   m_fInitialized = true; // GUIManager is now initialized
}

void GUIManager::Shutdown()
{
   assert( m_fInitialized == true && L"GUIManager is not initialized" );
   m_elements.clear(); // Clear all GUI elements
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void GUIManager::Attach( std::shared_ptr< IGUIElement > element )
{
   m_elements.push_back( std::move( element ) );
}

void GUIManager::Detach( const std::shared_ptr< IGUIElement >& element )
{
   m_elements.erase( std::remove( m_elements.begin(), m_elements.end(), element ), m_elements.end() );
}

void GUIManager::Draw()
{
   if( !m_elements.empty() )
   {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      for( const std::shared_ptr< IGUIElement >& e : m_elements )
         e->Draw();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
   }
}

// ========================================================================
//      DebugGUI
// ========================================================================
void DebugGUI::Draw()
{
   { // ImGui Top Left
      ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
      ImGui::Begin( "Debug",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
      ImGui::Text( "FPS: %.1f (%.1fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate );
      static bool fVSync = Window::Get().GetWindowData().VSync;
      if( ImGui::Checkbox( "VSync", &fVSync ) )
         Window::Get().SetVSync( fVSync );

      ImGui::Text( "Tick: %d", m_timestep.GetLastTick() );
      ImGui::Text( "Entity Count: %d", m_registry.GetEntityCount() );

      ImGui::NewLine();
      auto [ pPlayerTran, pPlayerVel ] = m_registry.GetComponents< CTransform, CVelocity >( m_player );
      if( pPlayerTran && pPlayerVel )
      {
         ImGui::Text( "Player Entity: %d", m_player );
         ImGui::Text( "Player Position: %.2f, %.2f, %.2f", pPlayerTran->position.x, pPlayerTran->position.y, pPlayerTran->position.z );
         ImGui::Text( "Player Rotation: %.2f, %.2f, %.2f", pPlayerTran->rotation.x, pPlayerTran->rotation.y, pPlayerTran->rotation.z );
         ImGui::Text( "Player Velocity: %.2f, %.2f, %.2f", pPlayerVel->velocity.x, pPlayerVel->velocity.y, pPlayerVel->velocity.z );
         //ImGui::Text( "Player Acceleration: %.2f, %.2f, %.2f", m_player.GetAcceleration().x, m_player.GetAcceleration().y, m_player.GetAcceleration().z );
      }

      if( CTransform* pCameraTran = m_registry.GetComponent< CTransform >( m_camera ) )
      {
         ImGui::NewLine();
         ImGui::Text( "Camera Entity: %d", m_camera );
         ImGui::Text( "Camera Position: %.2f, %.2f, %.2f", pCameraTran->position.x, pCameraTran->position.y, pCameraTran->position.z );
         ImGui::Text( "Camera Rotation: %.2f, %.2f, %.2f", pCameraTran->rotation.x, pCameraTran->rotation.y, pCameraTran->rotation.z );
      }
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

         AddKeyBinding( "Escape", "Toggle Mouse" );
         AddKeyBinding( "F1", "Quit" );
         AddKeyBinding( "P", "Toggle Wireframe" );
         ImGui::EndTable();
      }
      ImGui::End();
   }
}
