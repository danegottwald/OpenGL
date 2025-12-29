#include "MainMenuState.h"

// Project Dependencies
#include <Engine/Core/UI.h>

namespace Game
{

class MainMenuDrawable final : public UI::IDrawable
{
public:
   MainMenuDrawable( Engine::GameContext& ctx, bool& startGameRequested ) :
      m_ctx( ctx ),
      m_startGameRequested( startGameRequested )
   {}

   void Draw() override
   {
      const WindowState& ws = m_ctx.WindowRef().GetWindowState();

      const ImVec2 buttonSize( 220.0f, 60.0f );
      ImGui::SetNextWindowPos( ImVec2( ( ( float )ws.size.x - buttonSize.x ) * 0.5f, ( ( float )ws.size.y - buttonSize.y ) * 0.5f ), ImGuiCond_Always );
      ImGui::SetNextWindowSize( ImVec2( ( float )ws.size.x, ( float )ws.size.y ), ImGuiCond_Always );

      ImGui::Begin( "##MainMenuRoot",
                    nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                       ImGuiWindowFlags_NoBackground );
      if( ImGui::Button( "Start Game", buttonSize ) )
         m_startGameRequested = true;

      ImGui::End();
   }

private:
   Engine::GameContext& m_ctx;
   bool&                m_startGameRequested;
};


void MainMenuState::DrawUI( Engine::GameContext& ctx )
{
   ctx.WindowRef().GetUIC().Register( std::make_shared< MainMenuDrawable >( ctx, m_startGameRequested ) );
}

} // namespace Game
