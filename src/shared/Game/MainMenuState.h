#pragma once

#include <Game/GameState.h>

namespace Game
{

class MainMenuState final : public GameState
{
public:
   MainMenuState() = default;

   void OnEnter() override {}
   void OnExit() override {}

   void Update( float ) override {}
   void FixedUpdate( float ) override {}
   void Render() override {}

   void DrawUI( UI::UIContext& ui ) override;

   // Called by UI to request game start
   void RequestStartGame();
};

} // namespace Game
