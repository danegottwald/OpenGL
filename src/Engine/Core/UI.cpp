#include "UI.h"

// Local dependencies
#include "Window.h"

// Project dependencies
#include <Engine/Core/Time.h>
#include <Engine/ECS/Registry.h>
#include <Engine/ECS/Components/Transform.h>
#include <Engine/ECS/Components/Velocity.h>


namespace UI
{

/*static*/ void UIBuffer::Init()
{
   if( s_fInitialized )
      return;

   GLFWwindow* pNativeWindow = Window::GetNativeWindow();
   if( !pNativeWindow )
      throw std::runtime_error( "Ensure window is initialized before UIBuffer" );

   IMGUI_CHECKVERSION();                 // Ensure correct ImGui version
   ImGui::CreateContext();               // Create a new ImGui context
   ImGui::GetIO().IniFilename = nullptr; // Disable saving ImGui settings to a file
   ImGui::StyleColorsDark();             // Use dark theme for ImGui

   ImGui_ImplGlfw_InitForOpenGL( pNativeWindow, true ); // Initialize ImGui with GLFW support
   if( !ImGui_ImplOpenGL3_Init( "#version 330" ) )
      throw std::runtime_error( "Failed to initialize ImGui OpenGL backend" );

   s_uiElements.reserve( kInitialUIElementCapacity ); // Pre-allocate space for UI elements
   s_fInitialized = true;
}


/*static*/ void UIBuffer::Shutdown()
{
   if( !s_fInitialized )
      throw std::runtime_error( "UIBuffer::Shutdown - UIBuffer not initialized" );

   s_uiElements.clear();
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}


/*static*/ void UIBuffer::Register( std::shared_ptr< IDrawable > element )
{
   if( !s_fInitialized )
      throw std::runtime_error( "UIBuffer::Register - UIBuffer not initialized" );

   s_uiElements.push_back( std::move( element ) );
}


/*static*/ void UIBuffer::Draw()
{
   if( !s_fInitialized )
      throw std::runtime_error( "UIBuffer::Draw - UIBuffer not initialized" );

   if( !s_uiElements.empty() )
   {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      for( const std::shared_ptr< IDrawable >& e : s_uiElements )
         e->Draw();

      s_uiElements.clear();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
   }
}

} // namespace UI


#include <windows.h>
#include <psapi.h>

struct ProcessMemoryInfo
{
   size_t workingSetMB;     // physical RAM in use
   size_t privateMB;        // memory only your process owns (good for leaks)
   size_t peakWorkingSetMB; // peak physical RAM usage
};


static ProcessMemoryInfo GetProcessMemoryInfoMB()
{
   constexpr size_t           MB = 1024 * 1024;
   PROCESS_MEMORY_COUNTERS_EX pmc {};
   if( GetProcessMemoryInfo( GetCurrentProcess(), reinterpret_cast< PROCESS_MEMORY_COUNTERS* >( &pmc ), sizeof( pmc ) ) )
      return { pmc.WorkingSetSize / MB, pmc.PrivateUsage / MB, pmc.PeakWorkingSetSize / MB };

   return {};
}


// ========================================================================
// DebugUI
// ========================================================================
class DebugUI final : public UI::IDrawable
{
public:
   DebugUI( const Entity::Registry& registry, Entity::Entity player, Entity::Entity camera, const Time::FixedTimeStep& timestep ) :
      m_registry( registry ),
      m_player( player ),
      m_camera( camera ),
      m_timestep( timestep )
   {}

   void Draw() override;

private:
   const Entity::Registry&    m_registry;
   Entity::Entity             m_player;
   Entity::Entity             m_camera;
   const Time::FixedTimeStep& m_timestep;
};


void DebugUI::Draw()
{
   { // ImGui Top Left
      ImGui::SetNextWindowSize( ImVec2( 420, 250 ), ImGuiCond_FirstUseEver );
      ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
      ImGui::Begin( "Debug",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_AlwaysAutoResize );

      if( ImGui::BeginTabBar( "##DebugTabs" ) )
      {
         // =====================================================================
         // Stats / Overview Tab (original content)
         // =====================================================================
         if( ImGui::BeginTabItem( "Stats" ) )
         {
            ImGui::Text( "FPS: %.1f (%.1fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate );
            static bool fVSync = Window::Get().GetWindowData().VSync;
            if( ImGui::Checkbox( "VSync", &fVSync ) )
               Window::Get().SetVSync( fVSync );

            ImGui::Text( "Tick: %d", m_timestep.GetTickCount() );
            ImGui::Text( "Entity Count: %d", ( int )m_registry.GetEntityCount() );

            ImGui::NewLine();
            auto [ pPlayerTran, pPlayerVel ] = m_registry.TryGet< CTransform, CVelocity >( m_player );
            if( pPlayerTran && pPlayerVel )
            {
               ImGui::Text( "Player Entity: %d", m_player );
               ImGui::Text( "Player Position: %.2f, %.2f, %.2f", pPlayerTran->position.x, pPlayerTran->position.y, pPlayerTran->position.z );
               ImGui::Text( "Player Rotation: %.2f, %.2f, %.2f", pPlayerTran->rotation.x, pPlayerTran->rotation.y, pPlayerTran->rotation.z );
               ImGui::Text( "Player Velocity: %.2f, %.2f, %.2f", pPlayerVel->velocity.x, pPlayerVel->velocity.y, pPlayerVel->velocity.z );
            }

            if( const CTransform* pCameraTran = m_registry.TryGet< CTransform >( m_camera ) )
            {
               ImGui::NewLine();
               ImGui::Text( "Camera Entity: %d", m_camera );
               ImGui::Text( "Camera Position: %.2f, %.2f, %.2f", pCameraTran->position.x, pCameraTran->position.y, pCameraTran->position.z );
               ImGui::Text( "Camera Rotation: %.2f, %.2f, %.2f", pCameraTran->rotation.x, pCameraTran->rotation.y, pCameraTran->rotation.z );
            }

            ImGui::EndTabItem();
         }

         if( ImGui::BeginTabItem( "Memory" ) )
         {
            const ProcessMemoryInfo memInfo           = GetProcessMemoryInfoMB();
            static int              memIndex          = 0;
            static float            memHistory[ 128 ] = {}, peakMem = 0.0f;
            memHistory[ memIndex ] = ( float )memInfo.workingSetMB;
            if( memHistory[ memIndex ] > peakMem )
               peakMem = memHistory[ memIndex ];

            memIndex = ( memIndex + 1 ) % 128;

            ImGui::Text( "Current: %zu MB", memInfo.workingSetMB );
            ImGui::Text( "Private: %zu MB", memInfo.privateMB );
            ImGui::Text( "Peak:    %zu MB", memInfo.peakWorkingSetMB );
            ImGui::PlotLines( "Memory Usage (MB)",
                              memHistory,
                              128,
                              memIndex,
                              nullptr,        // overlay text
                              0.0f,           // min scale
                              peakMem * 1.1f, // max scale (slightly above peak)
                              ImVec2( 0, 80 ) // plot size
            );

            ImGui::EndTabItem();
         }

         // =====================================================================
         // Component Inspector Tab (entity selection + dynamic component list)
         // =====================================================================
         if( ImGui::BeginTabItem( "Components" ) )
         {
            static uint64_t selectedEntity = 0; // persists while app running
            const uint64_t  entityCount    = m_registry.GetEntityCount();
            if( entityCount == 0 )
            {
               ImGui::TextDisabled( "<No Entities>" );
               ImGui::EndTabItem();
               ImGui::EndTabBar();
               ImGui::End();
               return; // nothing else to show
            }

            uint64_t lastValidIndex = entityCount - 1; // clamp range 0..last
            ImGui::TextUnformatted( "Select Entity" );

            // Combo listing existing entities (always valid indices only)
            if( ImGui::BeginCombo( "##EntityCombo", std::format( "{}", selectedEntity ).c_str() ) )
            {
               for( uint64_t e = 0; e < entityCount; ++e )
               {
                  bool isSelected = ( e == selectedEntity );
                  if( ImGui::Selectable( std::format( "Entity {}", e ).c_str(), isSelected ) )
                     selectedEntity = e;

                  if( isSelected )
                     ImGui::SetItemDefaultFocus();
               }
               ImGui::EndCombo();
            }

            ImGui::SameLine();
            int tempInput = static_cast< int >( selectedEntity );
            ImGui::SetNextItemWidth( 80 );
            if( ImGui::InputInt( "##EntityInput", &tempInput, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll ) )
               selectedEntity = static_cast< uint64_t >( std::clamp( tempInput, 0, static_cast< int >( lastValidIndex ) ) );

            ImGui::SameLine();
            bool disablePrev = selectedEntity == 0;
            if( disablePrev )
               ImGui::BeginDisabled();
            if( ImGui::ArrowButton( "##PrevEntity", ImGuiDir_Left ) && selectedEntity > 0 )
               selectedEntity--;
            if( disablePrev )
               ImGui::EndDisabled();

            ImGui::SameLine();
            bool disableNext = selectedEntity >= lastValidIndex;
            if( disableNext )
               ImGui::BeginDisabled();
            if( ImGui::ArrowButton( "##NextEntity", ImGuiDir_Right ) && selectedEntity < lastValidIndex )
               selectedEntity++;
            if( disableNext )
               ImGui::EndDisabled();

            if( !m_registry.FValid( selectedEntity ) )
               ImGui::TextColored( ImVec4( 1, 0.5f, 0.4f, 1 ), "Entity %llu currently not alive", ( unsigned long long )selectedEntity );
            else
            {
               ImGui::Separator();
               size_t componentCount = m_registry.GetComponentCount( selectedEntity );
               if( ImGui::TreeNodeEx( std::format( "Entity {} ({} components)##entity_components_root", selectedEntity, componentCount ).c_str(),
                                      ImGuiTreeNodeFlags_DefaultOpen ) )
               {
                  if( componentCount == 0 )
                     ImGui::TextDisabled( "<No Components>" );
                  else
                  {
                     // hopefully make this more dynamic in the future so it can handle any component type
                     auto [ pTransform, pVelocity ] = m_registry.TryGet< CTransform, CVelocity >( selectedEntity );
                     if( pTransform && ImGui::TreeNodeEx( "CTransform", ImGuiTreeNodeFlags_DefaultOpen ) )
                     {
                        ImGui::Text( "Position: %.2f, %.2f, %.2f", pTransform->position.x, pTransform->position.y, pTransform->position.z );
                        ImGui::Text( "Rotation: %.2f, %.2f, %.2f", pTransform->rotation.x, pTransform->rotation.y, pTransform->rotation.z );
                        ImGui::TreePop();
                     }
                     if( pVelocity && ImGui::TreeNodeEx( "CVelocity", ImGuiTreeNodeFlags_DefaultOpen ) )
                     {
                        ImGui::Text( "Velocity: %.2f, %.2f, %.2f", pVelocity->velocity.x, pVelocity->velocity.y, pVelocity->velocity.z );
                        ImGui::TreePop();
                     }
                     //for( component : componentsOfEntity )
                     //{
                     //   if( ImGui::TreeNode( component.name ) )
                     //   {
                     //      ImGui::Text( "ComponentData: %.2f, %.2f, %.2f", < component data > );
                     //      ImGui::TreePop();
                     //   }
                     //}
                  }
                  ImGui::TreePop();
               }
            }

            ImGui::EndTabItem();
         }

         ImGui::EndTabBar();
      }

      ImGui::End(); // Debug window
   }

   { // ImGui Top Right
      ImGui::SetNextWindowPos( ImVec2( Window::Get().GetWindowData().Width - 10, 10 ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
      ImGui::Begin( "Key Bindings",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize );
      if( ImGui::BeginTable( "KeyBindingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
      {
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
         AddKeyBinding( "F11", "Toggle Fullscreen" );
         ImGui::EndTable();
      }
      ImGui::End();
   }
}


std::shared_ptr< UI::IDrawable > CreateDebugUI( const Entity::Registry&    registry,
                                                Entity::Entity             player,
                                                Entity::Entity             camera,
                                                const Time::FixedTimeStep& timestep )
{
   return std::make_shared< DebugUI >( registry, player, camera, timestep );
}
