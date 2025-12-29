#pragma once

#include <Game/Game.h>

namespace Game
{

class MainMenuState final : public Game
{
public:
   explicit MainMenuState( bool& startGameRequested ) noexcept :
      m_startGameRequested( startGameRequested )
   {}

   void OnEnter( Engine::GameContext& ) override {}
   void OnExit( Engine::GameContext& ) override {}

   void Update( Engine::GameContext&, float ) override {}
   void FixedUpdate( Engine::GameContext&, float ) override {}
   void Render( Engine::GameContext& ) override {}

   void DrawUI( Engine::GameContext& ctx ) override;

private:
   bool& m_startGameRequested;
};

} // namespace Game
