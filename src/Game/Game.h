#pragma once

#include <Engine/Core/GameContext.h>

namespace Game
{

class Game
{
public:
   virtual ~Game() = default;

   virtual void OnEnter( Engine::GameContext& ctx ) = 0;
   virtual void OnExit( Engine::GameContext& ctx )  = 0;

   virtual void Update( Engine::GameContext& ctx, float dt )      = 0;
   virtual void FixedUpdate( Engine::GameContext& ctx, float dt ) = 0;
   virtual void Render( Engine::GameContext& ctx )                = 0;

   virtual void DrawUI( Engine::GameContext& ctx ) = 0;
};

} // namespace Game
