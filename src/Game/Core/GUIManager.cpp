
#include "GUIManager.h"

#include "Window.h"

// ========================================================================
//      GUIManager
// ========================================================================
bool                                          GUIManager::m_fInitialized = false;
std::vector< std::shared_ptr< IGuiElement > > GUIManager::m_elements;

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

void GUIManager::Reset()
{
   assert( m_fInitialized == true && L"GUIManager is not initialized" );
   m_elements.clear(); // Clear all GUI elements
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void GUIManager::Attach( std::shared_ptr< IGuiElement > element )
{
   m_elements.push_back( element );
}

void GUIManager::Detach( std::shared_ptr< IGuiElement > element )
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

      for( const std::shared_ptr< IGuiElement >& e : m_elements )
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
      ImGui::Begin( "Performance",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
      ImGui::Text( "FPS: %.1f (%.1fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate );
      static bool fVSync = Window::Get().GetWindowData().VSync;
      if( ImGui::Checkbox( "VSync", &fVSync ) )
         Window::Get().SetVSync( fVSync );
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

   //{ // ImGui Bottom Left
   //   ImGui::SetNextWindowPos( ImVec2( 10, Window::Get().GetWindowData().Height - 10 ), ImGuiCond_Always, ImVec2( 0.0f, 1.0f ) );
   //   ImGui::Begin( "Debug",
   //                 nullptr,
   //                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
   //                    /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
   //   ImGui::Text( "Player Position: %.2f, %.2f, %.2f", player.GetPosition().x, player.GetPosition().y, player.GetPosition().z );
   //   ImGui::Text( "Player Rotation: %.2f, %.2f, %.2f", player.GetRotation().x, player.GetRotation().y, player.GetRotation().z );
   //   ImGui::Text( "Player Velocity: %.2f, %.2f, %.2f", player.GetVelocity().x, player.GetVelocity().y, player.GetVelocity().z );
   //   ImGui::Text( "Camera Position: %.2f, %.2f, %.2f", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z );
   //   ImGui::End();
   //}
}
