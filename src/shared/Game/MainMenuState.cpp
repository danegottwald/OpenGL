#include "MainMenuState.h"

#include <Engine/UI/UI.h>
#include <Game/InGameState.h>

namespace Game
{

class MainMenuDrawable final : public UI::IDrawable
{
public:
   MainMenuDrawable( Engine::GameContext& ctx, MainMenuState& menuState ) :
      m_ctx( ctx ),
      m_menuState( menuState )
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
         m_menuState.RequestStartGame();

      ImGui::End();
   }

private:
   Engine::GameContext& m_ctx;
   MainMenuState&       m_menuState;
};


void MainMenuState::DrawUI( UI::UIContext& ui )
{
   ui.Register( std::make_shared< MainMenuDrawable >( GameCtx(), *this ) );
}


void MainMenuState::RequestStartGame()
{
   SwitchState< InGameState >();
}

} // namespace Game
