
#include "GUIManager.h"

#include "Window.h"
#include "../Entity/Player.h"
#include "../Camera.h"

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
DebugGUI::DebugGUI( Player& player, Camera& camera ) :
   m_player( player ),
   m_camera( camera )
{}

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

      ImGui::NewLine();
      ImGui::Text( "Player Position: %.2f, %.2f, %.2f", m_player.GetPosition().x, m_player.GetPosition().y, m_player.GetPosition().z );
      ImGui::Text( "Player Rotation: %.2f, %.2f, %.2f", m_player.GetRotation().x, m_player.GetRotation().y, m_player.GetRotation().z );
      ImGui::Text( "Player Velocity: %.2f, %.2f, %.2f", m_player.GetVelocity().x, m_player.GetVelocity().y, m_player.GetVelocity().z );
      ImGui::Text( "Player Acceleration: %.2f, %.2f, %.2f", m_player.GetAcceleration().x, m_player.GetAcceleration().y, m_player.GetAcceleration().z );
      ImGui::Text( "Camera Position: %.2f, %.2f, %.2f", m_camera.GetPosition().x, m_camera.GetPosition().y, m_camera.GetPosition().z );
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

   struct ServerListItem
   {
      std::string name;
      int         playerCount;
      int         ping;
      bool        connectRequested = false;
   };

   // Add this to your DebugGUI class (or another GUI element as needed)
   std::vector< ServerListItem > m_serverList = {
      { "Alpha Server",   12, 45 },
      { "Bravo Server",   8,  60 },
      { "Charlie Server", 24, 30 },
      { "Delta Server",   5,  80 },
      { "Echo Server",    16, 55 },
      { "Foxtrot Server", 9,  70 },
      { "Golf Server",    20, 40 },
      { "Hotel Server",   7,  90 }  // ... add more as needed
   };

   { // ImGui Server List (fills window, no padding, no title)
      ImGui::SetNextWindowPos( ImVec2( 10, 200 ), ImGuiCond_Always );
      ImGui::SetNextWindowSize( ImVec2( 400, 250 ), ImGuiCond_Always );

      ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
      ImGui::Begin( "Server List", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

      if( ImGui::BeginTable( "ServerListTable",
                             4,
                             ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp ) )
      {
         ImGui::TableSetupColumn( "Server Name", ImGuiTableColumnFlags_WidthStretch );
         ImGui::TableSetupColumn( "Players", ImGuiTableColumnFlags_WidthFixed, 70.0f );
         ImGui::TableSetupColumn( "Ping", ImGuiTableColumnFlags_WidthFixed, 60.0f );
         ImGui::TableSetupColumn( "Action", ImGuiTableColumnFlags_WidthFixed, 90.0f );
         ImGui::TableHeadersRow();

         for( size_t i = 0; i < m_serverList.size(); ++i )
         {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            ImGui::TextUnformatted( m_serverList[ i ].name.c_str() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%d", m_serverList[ i ].playerCount );
            ImGui::TableSetColumnIndex( 2 );
            ImGui::Text( "%d ms", m_serverList[ i ].ping );
            ImGui::TableSetColumnIndex( 3 );
            std::string btnLabel = "Connect##" + std::to_string( i );
            if( ImGui::Button( btnLabel.c_str() ) )
            {
               m_serverList[ i ].connectRequested = true;
               // Handle connect logic here
            }
         }
         ImGui::EndTable();
      }
      ImGui::End();
      ImGui::PopStyleVar();
   }
}
